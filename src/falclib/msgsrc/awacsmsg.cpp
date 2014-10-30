#include <d3dtypes.h>
#include "MsgInc/AWACSMsg.h"
#include "mesg.h"
#include "simdrive.h"
#include "fsound.h"
#include "falcsnd/conv.h"
#include "aircrft.h"
#include "msginc/RadioChatterMsg.h"
#include "MsgInc/DivertMsg.h"
#include "airunit.h"
#include "vutypes.h"
#include "Find.h"
#include "Atm.h"
#include "radar.h"
#include "SMS.h"
#include "airframe.h"
#include "graphics/include/vmath.h"
#include "Navunit.h" // M.N. Vector to Carrier
#include "PlayerOp.h"
#include "MissEval.h"
#include "classtbl.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"

//sfr: added here for checks
#include "InvalidBufferException.h"

#pragma warning(disable:4786)
#include <map>
#include <set>
typedef std::map<float, FlightClass *> stdRange2FlightMap;
typedef std::set<FlightClass *> stdFlightSet;

#include "atcbrain.h"
#include "falclist.h"

extern void AircraftLaunch(Flight f);
extern int doUI;
int AWACSon = TRUE;

extern VU_TIME vuxGameTime;
extern bool g_bAWACSRequired;
extern int g_nMinTacanChannel;
extern int g_nChatterInterval; // FRB message interval time

VU_ID FindAircraftTarget(AircraftClass* theVehicle);  // 2001-10-26 ADDED BY S.G. Need it in request for help

FalconAWACSMessage::FalconAWACSMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent(AWACSMsg, FalconEvent::SimThread, entityId, target, loopback)
{
    // Your Code Goes Here
    //me123 RequestOutOfBandTransmit ();
    RequestReliableTransmit(); //me123
}

FalconAWACSMessage::FalconAWACSMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent(AWACSMsg, FalconEvent::SimThread, senderid, target)
{
    // Your Code Goes Here
    type;
}

FalconAWACSMessage::~FalconAWACSMessage(void)
{
    // Your Code Goes Here
}

static void CollectAssistCandidates(AircraftClass *plane, stdRange2FlightMap &arrCandidates)
{
    // loop thru all sim objects
    D3DFrame::Vector vPosPlane(plane->XPos(), plane->YPos(), plane->ZPos());
    FlightClass *pFlight;
    stdFlightSet setTmp;

    {
        VuListIterator unitWalker(SimDriver.objectList);
        SimBaseClass *p = (SimBaseClass*) unitWalker.GetFirst();

        while (p)
        {
            // Ignore objects under these conditions:
            ///////////////////////////////////////////////////////////
            // - This plane
            // - Member of this plane's flight
            // - Outside of maximum range
            // - Not a flight
            // - Not same team
            // - Not on a CAP or SWEEP mission
            // - Flight is engaged

            if ((p not_eq plane) and 
                (p->GetCampaignObject() not_eq plane->GetCampaignObject()) and 
                p->IsAirplane() and 
                (pFlight = (FlightClass *) p->GetCampaignObject()) and 
                (setTmp.find(pFlight) == setTmp.end()) and 
                (pFlight->GetAssignedTarget() == FalconNullId) and 
                (plane->GetTeam() == p->GetTeam()))
            {
                D3DFrame::Vector vPos(p->XPos(), p->YPos(), p->ZPos());
                float fRange = (vPos - vPosPlane).Size2D() / NM_TO_FT;
                const float fRangeMax = 50; // 50 nm

                if (fRange <= fRangeMax)
                {
                    MissionTypeEnum missionType = pFlight->GetUnitMission();

                    switch (missionType)
                    {
                        case AMIS_BARCAP: // BARCAP missions to protect a target area
                        case AMIS_BARCAP2: // BARCAP missions to defend a border
                        case AMIS_HAVCAP:
                        case AMIS_TARCAP:

                            // case AMIS_RESCAP:
                        case AMIS_AMBUSHCAP:
                        case AMIS_SWEEP:
                        case AMIS_ALERT:

                            // case AMIS_INTERCEPT:
                        case AMIS_ONCALLCAS: // On call CAS

                            // case AMIS_PRPLANCAS: // Pre planned CAS
                        case AMIS_CAS: // Immediate CAS

                            // case AMIS_RECON:
                        case AMIS_BDA:
                        case AMIS_PATROL:
                        case AMIS_RECONPATROL: // Recon for enemy ground vehicles
                        {
                            arrCandidates.insert(std::map<float, FlightClass *>::value_type(fRange, pFlight));
                            setTmp.insert(pFlight);
                            break;
                        }
                    }
                }
            }

            p = (SimBaseClass*) unitWalker.GetNext();
        }
    }
#ifdef _DEBUG
    MonoPrint("CollectAssistCandidates - Dumping map of potential candidates");

    stdRange2FlightMap::iterator it;

    for (it = arrCandidates.begin(); it not_eq arrCandidates.end(); it++)
    {
        char strFlightName[0x100];
        GetCallsign(it->second->callsign_id, it->second->callsign_num, strFlightName);
        MonoPrint("Flight %s, Range %.2f nm", strFlightName, it->first);
    }

    MonoPrint("CollectAssistCandidates - End of dump");
#endif
}

int FalconAWACSMessage::Process(uchar autodisp)
{
    float altitude;
    short X, Y;
    Flight flight = NULL;
    AircraftClass *plane = NULL;
    FalconEntity* otherThing = NULL; // 2002-02-28 REINSTATED BY S.G. Could be a sim or a campaign object
    // CampBaseClass* otherThing = NULL;
    FalconRadioChatterMessage* radioMessage = NULL;
    Objective airbase = NULL;
    Flight tankerFlight = NULL;
    AircraftClass *tanker = NULL;
    TaskForce carrier = NULL; // VectorToCarrier
    int randNum;
    ulong delay = 2 * CampaignSeconds; // default delay
    ulong delay1 = g_nChatterInterval * CampaignSeconds; // FRB - user-controlled delay

    if (autodisp)
        return 0;

    if ( not PlayerOptions.PlayerRadioVoice)
        delay = 500; // shorter

    MonoPrint("AWACS message #%d playing.\n", dataBlock.type);

    plane = (AircraftClass*)vuDatabase->Find(EntityId());

    if (plane and plane->IsSim())
    {
        flight = (Flight)plane->GetCampaignObject();

        if (g_bAWACSRequired)   // JPO - an awacs for this flight is required
        {
            Flight awacs = flight->GetFlightController();

            if (awacs == NULL) // 2001-09-23 ADDED BY M.N. no AWACS assigned to the package ?
            {
                //               -> check if there is an AWACS in the sky at all
                Unit nu, cf;
                VuListIterator myit(AllAirList);
                nu = (Unit) myit.GetFirst();

                while (nu)
                {
                    cf = nu;
                    nu = (Unit) myit.GetNext();

                    if ( not cf->IsFlight() or cf->IsDead())
                        continue;

                    // 2002-03-07 MN of course only AWACS from our team - doh
                    if (cf->GetUnitMission() == AMIS_AWACS and cf->GetTeam() == plane->GetTeam())
                    {
                        awacs = (Flight) cf;
                        break;
                    }
                }

                if (awacs == NULL)
                    return 0;
            }
        }

        switch (dataBlock.type)
        {
            case Unable:

                // SendCallToAWACS(plane, rcUNABLE);
                // KCK: Reply to any pending diverts
                if (plane->IsLocal())
                    CheckDivertStatus(DIVERT_REPLY_NO);

                //rcUNABLE
                break;

            case Wilco:

                // SendCallToAWACS(plane, rcCOPY);
                // KCK: Reply to any pending diverts
                if (plane->IsLocal())
                    CheckDivertStatus(DIVERT_REPLY_YES);

                //rcCOPY
                break;

            case Judy:
                // This is the flight's request.
                // SendCallToAWACS(plane, rcJUDY);
                radioMessage = new FalconRadioChatterMessage(FalconNullId , FalconLocalSession);
                radioMessage->dataBlock.to = MESSAGE_FOR_TEAM;
                radioMessage->dataBlock.from = plane->Id();
                radioMessage->dataBlock.voice_id = (uchar)flight->GetPilotVoiceID(plane->pilotSlot);
                radioMessage->dataBlock.message = rcJUDY;
                radioMessage->dataBlock.edata[0] = flight->callsign_id;
                radioMessage->dataBlock.edata[1] = (short)ConvertFlightNumberToCallNumber(flight->callsign_num);
                FalconSendMessage(radioMessage, FALSE);

                if (plane->IsLocal())
                {
                    AWACSon = not AWACSon;
                }

                //rcJUDY
                break;

            case RequestPicture:
            {
                // SendCallToAWACS (plane, rcPICTUREQUERY, FalconLocalSession);

                if (plane->IsLocal())
                {
                    AWACSon = TRUE;

                    FalconAWACSMessage *awacsMsg = new FalconAWACSMessage(plane->Id(), FalconLocalGame);
                    awacsMsg->dataBlock.type = GivePicture;
                    FalconSendMessage(awacsMsg, TRUE);
                }
            }
            break;

            case GivePicture:
            {
                if ( not AWACSon)
                    return 0;

                CampBaseClass *campThreat = NULL;
                SimBaseClass *theThreat = NULL;
                /*
                SimBaseClass *theThreat = SimDriver.FindNearestThreat (plane, &X, &Y, &altitude);
                if (theThreat)
                {
                 radioMessage = CreateCallFromAwacs (flight, rcPICTUREBRA, FalconLocalSession);
                 //for now 4 sounds better (also almost always true :)
                 radioMessage->dataBlock.edata[4] =4; //picture
                 //for now let's not play this part
                 radioMessage->dataBlock.edata[5] = -1; //split type
                 radioMessage->dataBlock.edata[6] = X;
                 radioMessage->dataBlock.edata[7] = Y;
                 radioMessage->dataBlock.edata[8] = FloatToInt32(altitude);
                 MonoPrint ("Making threat call\n");
                }
                else if(campThreat = SimDriver.FindNearestCampThreat (plane, &X, &Y, &altitude))
                {
                 radioMessage = CreateCallFromAwacs(flight, rcPICTUREBRA);
                 //for now 4 sounds better (also almost always true :)
                 radioMessage->dataBlock.edata[4] =4; //picture
                 //for now let's not play this part
                 radioMessage->dataBlock.edata[5] = -1; //split type
                 radioMessage->dataBlock.edata[6] = X;
                 radioMessage->dataBlock.edata[7] = Y;
                 radioMessage->dataBlock.edata[8] = FloatToInt32(altitude);
                }
                else */
                theThreat = SimDriver.FindNearestEnemyPlane(plane, &X, &Y, &altitude);

                if (theThreat)
                {
                    radioMessage = CreateCallFromAwacsPlane(plane, rcPICTUREBRA);
                    //for now 4 sounds better (also almost always true :)
                    radioMessage->dataBlock.edata[4] = 4; //picture
                    //for now let's not play this part
                    radioMessage->dataBlock.edata[5] = -1; //split type
                    radioMessage->dataBlock.edata[6] = X;
                    radioMessage->dataBlock.edata[7] = Y;
                    radioMessage->dataBlock.edata[8] = (short)FloatToInt32(altitude);
                    MonoPrint("Making threat call\n");
                }
                else
                {
                    campThreat = SimDriver.FindNearestCampEnemy(plane, &X, &Y, &altitude);

                    if (campThreat)
                    {
                        radioMessage = CreateCallFromAwacsPlane(plane, rcPICTUREBRA);
                        //for now 4 sounds better (also almost always true :)
                        radioMessage->dataBlock.edata[4] = 4; //picture
                        //for now let's not play this part
                        radioMessage->dataBlock.edata[5] = -1; //split type
                        radioMessage->dataBlock.edata[6] = X;
                        radioMessage->dataBlock.edata[7] = Y;
                        radioMessage->dataBlock.edata[8] = (short)FloatToInt32(altitude);
                    }
                    else
                    {
                        radioMessage = CreateCallFromAwacsPlane(plane, rcPICTURECLEAR, FalconLocalSession);
                        MonoPrint("No threats detected :->\n");
                    }
                }

                radioMessage->dataBlock.time_to_play = delay; //wait a little bit so the request will play first
                FalconSendMessage(radioMessage, FALSE);
            }
            break;

            case RequestHelp:
                if (flight)
                {
                    int numAircraft = 0;
                    // SendCallToAWACS(plane, rcVECTORTOTHREAT);
                    CampBaseClass *campThreat = NULL;
                    SimBaseClass *simThreat = NULL; //SimDriver.FindNearestEnemyPlane(plane, &X, &Y, &altitude);
                    stdRange2FlightMap arrCandidates;
                    FlightClass *pEnemyFlight = NULL;
                    FlightClass *pBestInterceptorFlight = NULL;
                    float fInterceptorRange = 0;

                    // First see if we are already targeting someone. Request help against him
                    VU_ID tgtId = FindAircraftTarget(plane);

                    if (tgtId)
                    {
                        FalconEntity* newTarg = (FalconEntity*)vuDatabase->Find(tgtId);

                        if (newTarg and (newTarg->IsFlight() or newTarg->IsAirplane()))
                        {
                            if (newTarg->IsFlight())
                                pEnemyFlight = (FlightClass *) newTarg;

                            else if (newTarg->IsAirplane())
                                pEnemyFlight = (FlightClass *)((SimBaseClass *)newTarg)->GetCampaignObject();
                        }
                    }

                    // Now look at what's around us if we weren't targeting someone ourself
                    if ( not pEnemyFlight)
                    {
                        simThreat = SimDriver.FindNearestThreat(plane, &X, &Y, &altitude);

                        if (simThreat and (simThreat->IsFlight() or simThreat->IsAirplane()))
                        {
                            if (simThreat->IsFlight())
                                pEnemyFlight = (FlightClass *) simThreat;

                            else if (simThreat->IsAirplane())
                                pEnemyFlight = (FlightClass *) simThreat->GetCampaignObject();
                        }

                        else
                        {
                            campThreat = SimDriver.FindNearestCampThreat(plane, &X, &Y, &altitude);

                            if (campThreat and (campThreat->IsFlight()))
                            {
                                if (campThreat->IsFlight())
                                    pEnemyFlight = (FlightClass *) campThreat;
                            }
                        }
                    }

                    if (pEnemyFlight and not pEnemyFlight->Aborted())
                    {
                        RequestIntercept(pEnemyFlight, plane->GetTeam(), RI_HELP);
                        // This is the awacs's response
                        radioMessage = CreateCallFromAwacs(flight, rcAIRCOVERSENT);
                        radioMessage->dataBlock.edata[4] = -1; // No ETA time calculatable at this time
                        radioMessage->dataBlock.time_to_play = delay; // Delay the response
                        FalconSendMessage(radioMessage, FALSE);
                    }

                    else
                    {
                        radioMessage = CreateCallFromAwacs(flight, rcNOTARGETS);
                        MonoPrint("No threats detected :->\n");
                    }
                }

                break;

            case RequestRelief:
                if (flight)
                {
                    int hasFuel = 0, hasWeaps = 0, role;
                    int hp;
                    SMSClass *sms = (SMSClass*) plane->GetSMS();

                    // KCK: This works fine for aggregates.. Not so fine otherwise
                    // hasWeaps = flight->HasWeapons();
                    // hasFuel = flight->HasFuel();
                    role = flight->GetUnitCurrentRole();

                    // KCK: Here's one which will base all of the decisions off the player's aircraft
                    if (sms)
                    {
                        for (hp = 1; hp < sms->NumHardpoints(); hp++)
                        {
                            if (role == ARO_CA)
                            {
                                if (sms->hardPoint[hp] and sms->hardPoint[hp]->weaponPointer and sms->hardPoint[hp]->Domain() bitand wdAir)
                                    hasWeaps++;
                            }
                            else if (role == ARO_S or role == ARO_GA or role == ARO_SB or role == ARO_SEAD)
                            {
                                if (sms->hardPoint[hp] and sms->hardPoint[hp]->weaponPointer and sms->hardPoint[hp]->Domain() bitand wdGround)
                                    hasWeaps++;
                            }
                            else if (role == ARO_ASW or role == ARO_ASHIP)
                            {
                                if (sms->hardPoint[hp] and sms->hardPoint[hp]->weaponPointer and sms->hardPoint[hp]->Domain() bitand wdGround)
                                    hasWeaps++;
                            }

                            // else
                            // hasWeaps++; // Non-combat roles always 'have weapons'
                        }
                    }

                    if (plane->af->Fuel() > flight->class_data->Fuel / 3)
                        hasFuel++;

                    // Pick what we say depending on our status
                    // if ( not hasWeaps)
                    // SendCallToAWACS(plane, rcENDCAPARMS);
                    // else
                    // SendCallToAWACS(plane, rcENDCAPFUEL);

                    // 2002-02-21 MN bugfix - need to get missioneval status flag to see if STATION_OVER is true
                    // Awac's reponse.
                    FlightDataClass *flight_ptr = NULL;
                    int meflags = 0;

                    flight_ptr = TheCampaign.MissionEvaluator->FindFlightData(flight);

                    if (flight_ptr)
                        meflags = flight_ptr->status_flags;

                    // What's that ?? Only true if FEVAL_GOT_TO_TARGET is true and false at the same time ??
                    // if ( not hasFuel or not hasWeaps or ((flight->GetEvalFlags() bitand FEVAL_MISSION_STARTED) and (flight->GetEvalFlags() bitand FEVAL_GOT_TO_TARGET) and not (flight->GetEvalFlags() bitand FEVAL_GOT_TO_TARGET)))
                    if ( not hasFuel or not hasWeaps or ((flight->GetEvalFlags() bitand FEVAL_MISSION_STARTED) and (flight->GetEvalFlags() bitand FEVAL_GOT_TO_TARGET) and (meflags bitand MISEVAL_FLIGHT_STATION_OVER)))
                    {
                        if (rand() % 2)
                            radioMessage = CreateCallFromAwacs(flight, rcRELIEVED);
                        else
                            radioMessage = CreateCallFromAwacs(flight, rcDISMISSED);

                        // Notify the mission evaluator that we're free to leave
                        TheCampaign.MissionEvaluator->RegisterRelief(flight);
                    }
                    else
                        radioMessage = CreateCallFromAwacs(flight, rcCAPNOTOVER);

                    radioMessage->dataBlock.time_to_play = delay; // Delay the response
                    FalconSendMessage(radioMessage, FALSE);
                }

                break;

            case RequestDivert:
                if (flight)
                {
                    // Flight requesting divert
                    // KCK: I don't think we have the speach to impliment this
                    // SendCallToAWACS(plane, rcREQUESTTASK);
                    flight->SetUnitPriority(0);
                }

                break;

            case RequestSAR:
                if (flight)
                {
                    // Flight requesting SAR
                    // SendCallToAWACS(plane, rcREQUESTDIVERT);
                    flight->SetUnitPriority(0);

                    // AWACS response
                    if (RequestSARMission(flight))
                        radioMessage = CreateCallFromAwacs(flight, rcSARENROUTE);
                    else
                        radioMessage = CreateCallFromAwacs(flight, rcUNABLE);

                    radioMessage->dataBlock.time_to_play = delay;
                    FalconSendMessage(radioMessage, FALSE);
                }

                break;

            case OnStation:
                if (flight)

                    //TJL 12/14/03 Begin the new Check-in/PseudoFAC code
                {

                    //TJL Check in should only respond/work for aircraft on ARO_GA type missions
                    // All others will be told "unable".
                    //MissionRequestClass mis;
                    Unit nu, cf;
                    {
                        VuListIterator myit(AllAirList);
                        nu = (Unit) myit.GetFirst();

                        while (nu)
                        {
                            cf = nu;
                            nu = (Unit) myit.GetNext();

                            if (cf->IsFlight() or cf->IsDead())
                                continue;
                        }
                    }
#if 0 // Retro 20May2004 - fixed logic

                    if (flight->GetUnitCurrentRole() not_eq ARO_GA and 
                        flight->GetUnitMission() not_eq (AMIS_ONCALLCAS or AMIS_PRPLANCAS or AMIS_CAS or AMIS_SAD or AMIS_INT or AMIS_BAI))
#else
                    if (flight->GetUnitCurrentRole() not_eq ARO_GA and 
                        ((flight->GetUnitMission() not_eq AMIS_ONCALLCAS) and 
                         (flight->GetUnitMission() not_eq AMIS_PRPLANCAS) and 
                         (flight->GetUnitMission() not_eq AMIS_CAS) and 
                         (flight->GetUnitMission() not_eq AMIS_SAD) and 
                         (flight->GetUnitMission() not_eq AMIS_INT) and 
                         (flight->GetUnitMission() not_eq AMIS_BAI)))
#endif // Retro 20May2004 - end
                    {
                        radioMessage = CreateCallFromAwacs(flight, rcUNABLE);
                        radioMessage->dataBlock.time_to_play = delay;
                        FalconSendMessage(radioMessage, FALSE);

                        break; // Not an appropriate A/G mission, good bye
                    }

                    // TJL If mission type is flagged for a divert, tell the campaign we can divert
                    if (MissionData[flight->GetUnitMission()].flags bitand AMIS_EXPECT_DIVERT)
                    {
                        flight->SetUnitPriority(0);
                        flight->SetEvalFlag(FLIGHT_ON_STATION);
                    }



                    /* delete for now
                    radioMessage = CreateCallToAWACS(plane, rcFACREADY);
                    radioMessage->dataBlock.time_to_play = 5*CampaignSeconds;
                    FalconSendMessage(radioMessage, FALSE);
                    */

                    //TJL Now Vector Flight to their Target
                    CampBaseClass *target =  NULL;

                    target = (CampBaseClass *)vuDatabase->Find(flight->GetAssignedTarget());

                    if ( not target)
                        target = (CampBaseClass *)vuDatabase->Find(flight->GetUnitMissionTargetID());

                    if ( not target)
                        target = (CampBaseClass *)vuDatabase->Find(flight->GetTargetID());

                    if ( not target)
                    {
                        //TJL AWACS tells you to hold at CP Alpha
                        radioMessage = CreateCallFromAwacsPlane(plane, rcHOLDATCP);
                        radioMessage->dataBlock.edata[0] = flight->callsign_id;
                        radioMessage->dataBlock.edata[1] = (short)ConvertFlightNumberToCallNumber(flight->callsign_num);
                        radioMessage->dataBlock.edata[4] = 0;
                        radioMessage->dataBlock.time_to_play = 5 * CampaignSeconds;
                        FalconSendMessage(radioMessage, FALSE);
                    }
                    //TJL If an Air Target, Ignore
                    else if (target->IsUnit() and target->GetDomain() == DOMAIN_AIR)
                    {
                        //TJL AWACS tells you to hold at CP Alpha
                        radioMessage = CreateCallFromAwacsPlane(plane, rcHOLDATCP);
                        radioMessage->dataBlock.edata[0] = flight->callsign_id;
                        radioMessage->dataBlock.edata[1] = (short)ConvertFlightNumberToCallNumber(flight->callsign_num);
                        radioMessage->dataBlock.edata[4] = 0;
                        radioMessage->dataBlock.time_to_play = 5 * CampaignSeconds;
                        FalconSendMessage(radioMessage, FALSE);
                    }
                    else
                    {
                        radioMessage = CreateCallFromAwacs(flight, rcATTACKMYTARGET);
                        radioMessage->dataBlock.edata[5] = SimToGrid(target->YPos());
                        radioMessage->dataBlock.edata[6] = SimToGrid(target->XPos());
                    }

                    radioMessage->dataBlock.time_to_play = 10 * CampaignSeconds;
                    FalconSendMessage(radioMessage, FALSE);


                }

                break;

            case OffStation:
                if (flight)
                {
                    // KCK: I don't think we have the speach to impliment this
                    // SendCallToAWACS(plane, rcFACCONTACT);
                    // VWF: It seems we dont have a "check out" call. Vamoose is as
                    // close as it gets.
                    // AWACS/FAC callsign
                    // SendCallToAWACS(plane, rcVAMOOSE);

                    flight->ClearEvalFlag(FLIGHT_ON_STATION);

                    /* radioMessage = new FalconRadioChatterMessage( FalconNullId , FalconLocalSession );
                     radioMessage->dataBlock.to = MESSAGE_FOR_TEAM;
                     radioMessage->dataBlock.from = plane->Id();
                     radioMessage->dataBlock.voice_id = (uchar)flight->GetPilotVoiceID(plane->pilotSlot);
                     radioMessage->dataBlock.message = rcVAMOOSE;
                     radioMessage->dataBlock.edata[0] = -1;
                     radioMessage->dataBlock.edata[1] = -1;
                     FalconSendMessage(radioMessage, FALSE);*/
                    if (MissionData[flight->GetUnitMission()].flags bitand AMIS_EXPECT_DIVERT)
                        flight->SetDiverted(1);

                    // OW: Acknowledge
                    // radioMessage = CreateCallFromAwacs(flight, rcROGER);
                    radioMessage = CreateCallFromAwacs(flight, rcCOPY); // rcROGER is broken somehow
                    radioMessage->dataBlock.time_to_play = delay;
                    FalconSendMessage(radioMessage, FALSE);
                }

                break;

            case VectorHome:
                // SendCallToAWACS(plane, rcVECTORHOMEPLATE);
                //if not you must be homeless

                airbase = (Objective)vuDatabase->Find(plane->HomeAirbase());

                if (flight and airbase)
                {
                    //awacs response
                    radioMessage = CreateCallFromAwacs(flight, rcVECTORHOME);

                    radioMessage->dataBlock.time_to_play = delay;//wait a little bit so the request will play first

                    radioMessage->dataBlock.edata[4] = SimToGrid(airbase->YPos());
                    radioMessage->dataBlock.edata[5] = SimToGrid(airbase->XPos());

                    FalconSendMessage(radioMessage, FALSE);
                }
                else if (flight)
                {
                    radioMessage = CreateCallFromAwacs(flight, rcUNABLE);
                    radioMessage->dataBlock.time_to_play = delay;
                    FalconSendMessage(radioMessage, FALSE);
                }

                break;

            case VectorToAltAirfield:
                if (flight)
                {

#if 1
                    FalconRadioChatterMessage *radioMessage;

                    // Flight requesting divert field
                    // SendCallToAWACS(plane, rcDIVERTFIELD);

                    // Awacs response
                    radioMessage = CreateCallFromAwacs(flight, rcVECTORALTERNATE);

                    // KCK: No alternative airstrip.. should we try finding one? ATC List?
                    // TODO
                    // OW - here it is :)

                    vector pos;
                    pos.x = plane->XPos();
                    pos.y = plane->YPos();
                    GridIndex X = 0, Y = 0;
                    ConvertSimToGrid(&pos, &X, &Y);
                    ObjectiveClass *divertBase = FindNearestFriendlyAirbase(plane->GetTeam(), X, Y);

                    if (divertBase)
                    {
                        pos.x = divertBase->XPos();
                        pos.y = divertBase->YPos();
                        ConvertSimToGrid(&pos, &radioMessage->dataBlock.edata[4], &radioMessage->dataBlock.edata[5]);
                    }

                    else
                    {
                        radioMessage->dataBlock.edata[4] = 0;
                        radioMessage->dataBlock.edata[5] = 0;
                    }

                    if (flight->GetAWACSFlight())
                    {
                        radioMessage->dataBlock.from = flight->GetAWACSFlight()->Id();
                        radioMessage->dataBlock.voice_id = (uchar)(flight->GetAWACSFlight())->GetFlightLeadVoiceID();
                        radioMessage->dataBlock.edata[2] = (flight->GetAWACSFlight())->callsign_id;
                    }
                    else
                    {
                        radioMessage->dataBlock.from = FalconNullId;
                        radioMessage->dataBlock.voice_id = GetDefaultAwacsVoice(); // JPO VOICEFIX
                        radioMessage->dataBlock.edata[2] = gDefaultAWACSCallSign;
                    }

                    radioMessage->dataBlock.time_to_play = delay; // Delay the response
                    FalconSendMessage(radioMessage, FALSE);
                }

#else
                    WayPoint w = flight->GetFirstUnitWP();
                    FalconRadioChatterMessage *radioMessage;

                    // Flight requesting divert field
                    // SendCallToAWACS(plane, rcDIVERTFIELD);

                    // Awacs response
                    radioMessage = CreateCallFromAwacs(flight, rcVECTORALTERNATE);

                    while (w and not (w->GetWPFlags() bitand WPF_ALTERNATE))
                        w = w->GetNextWP();

                    if (w)
                        w->GetWPLocation(&radioMessage->dataBlock.edata[4], &radioMessage->dataBlock.edata[5]);
                    else
                    {
                        // KCK: No alternative airstrip.. should we try finding one? ATC List?
                        // TODO
                        radioMessage->dataBlock.edata[4] = 0;
                        radioMessage->dataBlock.edata[5] = 0;
                    }

                    if (flight->GetAWACSFlight())
                    {
                        radioMessage->dataBlock.from = flight->GetAWACSFlight()->Id();
                        radioMessage->dataBlock.voice_id = (uchar)(flight->GetAWACSFlight())->GetFlightLeadVoiceID();
                        radioMessage->dataBlock.edata[2] = (flight->GetAWACSFlight())->callsign_id;
                    }
                    else
                    {
                        radioMessage->dataBlock.from = FalconNullId;
                        radioMessage->dataBlock.voice_id = GetDefaultAwacsVoice(); // JPO VOICEFIX
                        radioMessage->dataBlock.edata[2] = gDefaultAWACSCallSign;
                    }

                    radioMessage->dataBlock.time_to_play = delay; // Delay the response
                    FalconSendMessage(radioMessage, FALSE);
                }

#endif
                break;

            case VectorToPackage:
                if (flight)
                {
                    // SendCallToAWACS(plane, rcVECTORTOPACKAGE);

                    Flight leadElement = NULL;
                    Package pack = (Package)flight->GetUnitParent();

                    if (pack)
                        leadElement = (Flight)pack->GetFirstUnitElement();

#if 0

                    radioMessage = CreateCallFromAwacs(flight, rcVECTORTOFLIGHT);
                    radioMessage->dataBlock.time_to_play = delay;//wait a little bit so the request will play first

                    //MI fix if all got shot down
                    if (leadElement)
                    {
                        radioMessage->dataBlock.edata[4] = SimToGrid(leadElement->YPos());
                        radioMessage->dataBlock.edata[5] = SimToGrid(leadElement->XPos());

                    }

                    else
                    {
                        radioMessage->dataBlock.edata[4] = SimToGrid(flight->YPos());
                        radioMessage->dataBlock.edata[5] = SimToGrid(flight->XPos());
                    }

#else

                    if (flight->GetACCount() > 1 and leadElement)
                    {
                        radioMessage = CreateCallFromAwacs(flight, rcVECTORTOFLIGHT);
                        radioMessage->dataBlock.time_to_play = delay;//wait a little bit so the request will play first
                        radioMessage->dataBlock.edata[4] = SimToGrid(leadElement->YPos());
                        radioMessage->dataBlock.edata[5] = SimToGrid(leadElement->XPos());
                    }
                    else if (flight->GetACCount() > 1)
                    {
                        radioMessage = CreateCallFromAwacs(flight, rcVECTORTOFLIGHT);
                        radioMessage->dataBlock.time_to_play = delay;//wait a little bit so the request will play first
                        radioMessage->dataBlock.edata[4] = SimToGrid(flight->YPos());
                        radioMessage->dataBlock.edata[5] = SimToGrid(flight->XPos());
                    }
                    else
                    {
                        //nobody there. could do a better response... but which one?
                        radioMessage = CreateCallFromAwacs(flight, rcUNABLE);
                        radioMessage->dataBlock.time_to_play = delay;//wait a little bit so the request will play first
                        radioMessage->dataBlock.edata[4] = rand() % 2;
                    }

#endif
                    FalconSendMessage(radioMessage, FALSE);
                }

                break;

            case VectorToTanker:
                // SendCallToAWACS (plane, rcREQUESTVECTORTOTANKER);

                tankerFlight = flight->GetTankerFlight();

                if ( not tankerFlight)
                {
                    tankerFlight = SimDriver.FindTanker(plane);
                }


                if (tankerFlight)
                {
                    tanker = (AircraftClass*)tankerFlight->GetComponentLead();

                    radioMessage = CreateCallFromAwacs(flight, rcVECTORTOTANKER);
                    radioMessage->dataBlock.message = rcVECTORTOTANKER;
                    radioMessage->dataBlock.edata[4] = tankerFlight->callsign_id;
                    radioMessage->dataBlock.edata[5] = (short)ConvertFlightNumberToCallNumber(tankerFlight->callsign_num);

                    if (tanker)
                    {
                        radioMessage->dataBlock.edata[6] = SimToGrid(tanker->YPos());
                        radioMessage->dataBlock.edata[7] = SimToGrid(tanker->XPos());
                    }
                    else
                    {
                        radioMessage->dataBlock.edata[6] = SimToGrid(tankerFlight->YPos());
                        radioMessage->dataBlock.edata[7] = SimToGrid(tankerFlight->XPos());
                    }

                    radioMessage->dataBlock.edata[8] = (short)(tankerFlight->tacan_channel - g_nMinTacanChannel);
                    radioMessage->dataBlock.edata[9] = 24;
                }
                else
                {
                    radioMessage = CreateCallFromAwacs(flight, rcNOTANKER);
                }

                radioMessage->dataBlock.time_to_play = delay; //wait a little bit so the request will play first
                FalconSendMessage(radioMessage, FALSE);

                break;

            case VectorToCarrier:

                // 2002-03-07 MN search for closest carrier instead of the home carrier
                if (flight)
                {
                    Unit nu, cf;
                    float dist, bestdist = 99999.9F, dx, dy;
                    {
                        VuListIterator myit(AllRealList);
                        nu = (Unit) myit.GetFirst();

                        while (nu)
                        {
                            cf = nu;
                            nu = (Unit) myit.GetNext();

                            if (cf->GetTeam() not_eq plane->GetTeam() or cf->IsDead())
                                continue;

                            // RV - Biker - All naval units return TRUE here so better check for subtype
                            if ( not cf->IsTaskForce() or cf->GetSType() not_eq STYPE_UNIT_CARRIER)
                                continue;

                            dx = plane->XPos() - cf->XPos();
                            dy = plane->YPos() - cf->YPos();
                            dist = (float)sqrt(dx * dx + dy * dy);

                            if (dist < bestdist)
                            {
                                bestdist = dist;
                                carrier = (TaskForce)cf;
                            }
                        }
                    }

                    if (carrier)
                    {
                        //awacs response
                        radioMessage = CreateCallFromAwacs(flight, rcVECTORHOMECARRIER);

                        radioMessage->dataBlock.time_to_play = delay;//wait a little bit so the request will play first

                        radioMessage->dataBlock.edata[4] = SimToGrid(carrier->YPos());
                        radioMessage->dataBlock.edata[5] = SimToGrid(carrier->XPos());
                        radioMessage->dataBlock.edata[6] = (short)(carrier->tacan_channel - g_nMinTacanChannel);
                        radioMessage->dataBlock.edata[7] = 24; // Yankee

                        FalconSendMessage(radioMessage, FALSE);
                    }
                    else
                    {
                        radioMessage = CreateCallFromAwacs(flight, rcUNABLE);
                        radioMessage->dataBlock.time_to_play = delay;
                        FalconSendMessage(radioMessage, FALSE);
                    }
                }

                break;

            case VectorToThreat:
                if (flight)
                {
                    int numAircraft = 0;
                    // SendCallToAWACS(plane, rcVECTORTOTHREAT);
                    CampBaseClass *campThreat = NULL;
                    SimBaseClass *simThreat = SimDriver.FindNearestThreat(plane, &X, &Y, &altitude);

                    if (simThreat)
                    {
                        numAircraft = simThreat->GetCampaignObject()->NumberOfComponents();
                        radioMessage = CreateCallFromAwacs(flight, rcNEARESTTHREATRSP);

                        //for this request we just want the BRA part
                        if (numAircraft > 1)
                            radioMessage->dataBlock.edata[4] = (short)((simThreat->Type() - VU_LAST_ENTITY_TYPE) * 2 + 1); //type
                        else
                            radioMessage->dataBlock.edata[4] = (short)((simThreat->Type() - VU_LAST_ENTITY_TYPE) * 2); //type

                        radioMessage->dataBlock.edata[5] = (short)numAircraft; //number
                        radioMessage->dataBlock.edata[6] = X;
                        radioMessage->dataBlock.edata[7] = Y;
                        radioMessage->dataBlock.edata[8] = (short)FloatToInt32(altitude);

                    }
                    else
                    {
                        campThreat = SimDriver.FindNearestCampThreat(plane, &X, &Y, &altitude);

                        if (campThreat)
                        {
                            numAircraft = campThreat->NumberOfComponents();
                            radioMessage = CreateCallFromAwacs(flight, rcNEARESTTHREATRSP);

                            //for this request we just want the BRA part
                            if (numAircraft > 1)
                                radioMessage->dataBlock.edata[4] = (short)(((Unit)campThreat)->GetVehicleID(0) * 2 + 1); //type
                            else
                                radioMessage->dataBlock.edata[4] = (short)(((Unit)campThreat)->GetVehicleID(0) * 2); //type

                            radioMessage->dataBlock.edata[5] = (short)numAircraft; //number
                            radioMessage->dataBlock.edata[6] = X;
                            radioMessage->dataBlock.edata[7] = Y;
                            radioMessage->dataBlock.edata[8] = (short)FloatToInt32(altitude);
                        }
                        else
                        {
                            radioMessage = CreateCallFromAwacs(flight, rcNOTARGETS);

                            MonoPrint("No threats detected :->\n");
                        }
                    }

                    radioMessage->dataBlock.time_to_play = delay;//wait a little bit so the request will play first
                    FalconSendMessage(radioMessage, FALSE);
                }

                break;

            case VectorToTarget:
                if (flight)
                {
                    CampBaseClass *target =  NULL;

                    // This is the flight's request
                    // SendCallToAWACS(plane, rcVECTORTOTARGET);

                    // This is AWAC's response
                    target = (CampBaseClass *)vuDatabase->Find(flight->GetAssignedTarget());

                    if ( not target)
                        target = (CampBaseClass *)vuDatabase->Find(flight->GetUnitMissionTargetID());

                    if ( not target)
                        target = (CampBaseClass *)vuDatabase->Find(flight->GetTargetID());

                    if ( not target)
                    {
                        radioMessage = CreateCallFromAwacs(flight, rcNOTARGETS);
                    }
                    else if (target->IsUnit() and target->GetDomain() == DOMAIN_AIR)
                    {
                        radioMessage = CreateCallFromAwacs(flight, rcAIRTARGETBRA);
                        target->GetLocation(&radioMessage->dataBlock.edata[4], &radioMessage->dataBlock.edata[5]);
                        radioMessage->dataBlock.edata[6] = (short)((Unit)target)->GetUnitAltitude();
                    }
                    else
                    {
                        radioMessage = CreateCallFromAwacs(flight, rcGNDTARGETBR);
                        target->GetLocation(&radioMessage->dataBlock.edata[4], &radioMessage->dataBlock.edata[5]);
                    }

                    radioMessage->dataBlock.time_to_play = delay;
                    FalconSendMessage(radioMessage, FALSE);
                }

                break;

            case CampAircraftLaunch:
                break;

            case CampFriendliesEngaged:
                break;

            case CampDivertCover:
            case CampDivertIntercept:
            case CampDivertCAS:
            case CampDivertOther:
            case CampDivertDenied:
                // Notify player
                break;

            case DeclareAircraft:
                // Need to send messages here to ask AWACS what the thing under my cursor is,
                // as well as play his response. Both of these are currently unknown to me.
                // SendCallToAWACS(plane, rcDECLARE);
                // 2002-02-28 MODIFIED BY S.G. Replaced CampBaseClass to FalconEntity since we
                // don't know if it's a sim or campaign object yet
                otherThing = (FalconEntity *)vuDatabase->Find(dataBlock.caller);

                if ( not otherThing)
                {
                    RadarClass* theRadar = (RadarClass*)FindSensor(plane, SensorClass::Radar);
                    VU_ID targetId;

                    if (theRadar)
                    {
                        targetId = theRadar->TargetUnderCursor();

                        if (targetId == FalconNullId and theRadar->CurrentTarget() and theRadar->CurrentTarget()->BaseData())
                        {
                            targetId = theRadar->CurrentTarget()->BaseData()->Id();
                        }
                    }

                    if (targetId)
                    {
                        otherThing = (CampBaseClass*)vuDatabase->Find(targetId);
                    }
                }

                // 2002-02-25 MN let AWACS say the exact type if identified
                if (otherThing)
                {

                    // 2002-02-28 modified by MN - get simbaseclass pointer, too, if SIM entity
                    // 2002-02-28 ADDED BY S.G. Needs to check if otherThing is a campaign or sim object
                    CampBaseClass* campThing;
                    SimBaseClass* simThing;

                    if (otherThing->IsSim())
                    {
                        simThing = (SimBaseClass *)otherThing;
                        campThing = ((SimBaseClass *)otherThing)->GetCampaignObject();
                    }
                    else
                    {
                        campThing = (CampBaseClass *)otherThing;
                        simThing  = 0; // MLR 2003-11-20 - CTD because this may not get initialized below.

                    }

                    // END OF ADDED SECTION

                    if (campThing and campThing->GetIdentified(plane->GetTeam()))
                    {

                        // flight and got identified (only flights can be "GetIdentified = true",
                        // so this checks also if it is a flight/aircraft)
                        radioMessage = CreateCallFromAwacsPlane(plane, rcDECLAREIDENTIFIEDTARGET);

                        if (GetTTRelations(otherThing->GetTeam(), plane->GetTeam()) < Hostile)
                        {
                            radioMessage->dataBlock.message = rcGENERALID;

                            // If friendly, 1% chance of being wrong
                            if (rand() % 100 == 0)
                            {
                                randNum = rand() % 100;

                                if (randNum < 80)
                                {
                                    radioMessage->dataBlock.edata[4] = 3;
                                }
                                else
                                {
                                    radioMessage->dataBlock.edata[4] = 2;
                                }
                            }
                            else
                            {
                                radioMessage->dataBlock.edata[4] = 4;
                            }
                        }
                        else
                        {

                            // 0 = Hostile  - Known bad guy
                            // 1 = Bandit   - bad guy known from point of origin
                            // 2 = Outlaw   - Probably bad based on speed and direction
                            // 3 = Bogie    - Unknown
                            // 4 = Freindly
                            randNum = rand() % 100;

                            if (otherThing->ZPos() > -3000)
                            {
                                radioMessage->dataBlock.edata[4] = 3;
                                radioMessage->dataBlock.message = rcGENERALID;
                            }
                            else if (randNum < 5)
                            {
                                radioMessage->dataBlock.edata[4] = 2;
                                radioMessage->dataBlock.message = rcGENERALID;
                            }
                            else if (randNum < 20 and GetTTRelations(otherThing->GetTeam(), plane->GetTeam()) < War)
                            {
                                radioMessage->dataBlock.edata[4] = 1;
                            }
                            else
                            {
                                radioMessage->dataBlock.edata[4] = 0;
                            }
                        }

                        if (simThing)
                        {
                            // MLR 2003-11-20 got a CTD here because simThing was 0x3, added initializing to 0 above.
                            // in Sim we always have only one aircraft locked or under the cursor
                            radioMessage->dataBlock.edata[5] =
                                (short)((simThing->Type() - VU_LAST_ENTITY_TYPE) * 2); //type
                        }
                        else
                        {
                            // A camp target can consist of more than one component
                            int numAircraft = campThing->NumberOfComponents();

                            if (numAircraft > 1)
                            {
                                //type
                                radioMessage->dataBlock.edata[5] = (short)(((Unit)campThing)->GetVehicleID(0) * 2 + 1);
                            }
                            else
                            {
                                //type
                                radioMessage->dataBlock.edata[5] = (short)(((Unit)campThing)->GetVehicleID(0) * 2);
                            }
                        }
                    }
                    else
                    {
                        // unidentified flights and others
                        // 2002-02-11 MN changed to CreateCallFromAwacsPlane -
                        //AWACS issues the exact aircraft ID in the flight (not just the flight number)
                        radioMessage = CreateCallFromAwacsPlane(plane, rcGENERALID);

                        if (GetTTRelations(otherThing->GetTeam(), plane->GetTeam()) < Hostile)
                        {
                            // If friendly, 1% chance of being wrong
                            if (rand() % 100 == 0)
                            {
                                randNum = rand() % 100;

                                if (randNum < 80)
                                    radioMessage->dataBlock.edata[4] = 3;
                                else
                                    radioMessage->dataBlock.edata[4] = 2;
                            }
                            else
                            {
                                radioMessage->dataBlock.edata[4] = 4;
                            }
                        }
                        else
                        {
                            // 0 = Hostile  - Known bad guy
                            // 1 = Bandit   - bad guy known from point of origin
                            // 2 = Outlaw   - Probably bad based on speed and direction
                            // 3 = Bogie    - Unknown
                            // 4 = Freindly
                            randNum = rand() % 100;

                            if (otherThing->ZPos() > -3000)
                                radioMessage->dataBlock.edata[4] = 3;
                            else if (randNum < 5)
                                radioMessage->dataBlock.edata[4] = 2;
                            else if (randNum < 20 and GetTTRelations(otherThing->GetTeam(), plane->GetTeam()) < War)
                                radioMessage->dataBlock.edata[4] = 1;
                            else
                                radioMessage->dataBlock.edata[4] = 0;
                        }
                    }
                }
                else
                {
                    radioMessage = CreateCallFromAwacs(flight, rcNOTARGETS);
                    MonoPrint("No threats detected :->\n");
                }

                radioMessage->dataBlock.time_to_play = delay;//wait a little bit so the request will play first
                FalconSendMessage(radioMessage, FALSE);
                MonoPrint("Ask for declare here\n");
                MonoPrint("Get declaration here\n");
                break;
        }
    }
    return 0;
}

// ==============================
// Support functions
// ==============================
