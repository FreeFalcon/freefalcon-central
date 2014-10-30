/*
 * Machine Generated source file for message "Divert Message".
 * NOTE: The functions here must be completed by hand.
 * Generated on 28-April-1997 at 22:39:35
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */

#include "MsgInc/DivertMsg.h"
#include "MsgInc/RadioChatterMsg.h"
#include "MsgInc/AWACSMsg.h"

#include "simdrive.h"
#include "falcuser.h"
#include "mesg.h"
#include "Mission.h"
#include "CmpClass.h"
#include "aircrft.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "campbase.h"
#include "flight.h"
#include "classtbl.h"
#include "dispcfg.h"
#include "gndai.h"


//sfr: added here for checks
#include "InvalidBufferException.h"

#define RESEND_DIVERT_TIME 25 // In seconds

// ==============================
// Globals
// ==============================

VU_ID gInterceptersId;
extern int doUI;

// ==============================
// Statics
// ==============================

static FalconDivertMessage sLastDivert;
static VU_ID sDivertFlight;
static int sLastReply = DIVERT_NO_DIVERT;
static CampaignTime sNextRepost = 0;

// ==============================
// Class Functions
// ==============================

FalconDivertMessage::FalconDivertMessage(void) : FalconEvent(DivertMsg, FalconEvent::CampaignThread, FalconNullId, NULL, FALSE)
{
    dataBlock.tot = 0;
    dataBlock.flags = 0;
    dataBlock.tx = dataBlock.ty = 0;
    dataBlock.mission = 0;
}

FalconDivertMessage::FalconDivertMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent(DivertMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{
    dataBlock.tot = 0;
    dataBlock.flags = 0;
    dataBlock.tx = dataBlock.ty = 0;
    dataBlock.mission = 0;
}

FalconDivertMessage::FalconDivertMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent(DivertMsg, FalconEvent::CampaignThread, senderid, target)
{
    dataBlock.tot = 0;
    dataBlock.flags = 0;
    dataBlock.tx = dataBlock.ty = 0;
    dataBlock.mission = 0;
    type;
}

FalconDivertMessage::~FalconDivertMessage(void)
{
    // Your Code Goes Here
}

int FalconDivertMessage::Process(uchar autodisp)
{
    Flight flight = (Flight)FindUnit(EntityId());
    CampEntity target = NULL;

    if (autodisp or not flight or not TheCampaign.IsLoaded())
        return 0;

    if (dataBlock.mission > 0)
    {
        // Check for target viability
        target = (CampEntity)vuDatabase->Find(dataBlock.targetID);

        if (target and target->IsUnit() and ((Unit)target)->Father())
            target = ((Unit)target)->GetFirstUnitElement();

        if (( not target or (target->IsUnit() and ((UnitClass*)target)->IsDead())) and dataBlock.mission > 0)
            return 0;

        // Set with new element's ID
        dataBlock.targetID = target->Id();

        // Make radio call
        PlayDivertRadioCalls(target, dataBlock.mission, flight, 0);

        // Return receipt
        if (flight->IsLocal() and (dataBlock.flags bitand REQF_NEEDRESPONSE))
        {
            CampEntity e = (CampEntity) vuDatabase->Find(dataBlock.requesterID);

            if (e->IsUnit())
                ((Unit)e)->SendUnitMessage(dataBlock.targetID, FalconUnitMessage::unitRequestMet, dataBlock.mission, flight->GetTeam(), 0);
        }

        // KCK: This is kinda hackish - Basically, for player leads, keep reposting this message
        // (every few seconds) until the player replies.
        if (flight->GetComponentLead() == FalconLocalSession->GetPlayerEntity() and flight == FalconLocalSession->GetPlayerFlight())
        {
            // Trying to track down a potential bug here.. It's hard enough to
            // get diverts I figure I'll let QA do the testing..
            // ShiAssert ( not "Show this to Kevin K.");
            memcpy(&sLastDivert.dataBlock, &dataBlock, sizeof(dataBlock));
            sDivertFlight = flight->Id();
            sLastReply = DIVERT_WAIT_FOR_REPLY;
            sNextRepost = vuxGameTime + RESEND_DIVERT_TIME * CampaignSeconds;
            flight->SetDiverted(1); // Set diverted to prevent additional calls
            return 0;
        }

        // Apply the divert
        ApplyDivert(flight, this);
    }
    else
        PlayDivertRadioCalls(NULL, DIVERT_DENIGNED, flight, 0);

    return 1;
}

// ===============================================
// Support functions
// ===============================================

// Returns -1 if no divert was pending, 0 if divert is no longer valid, 1 if reply accepted
int CheckDivertStatus(int reply)
{
    if ( not sNextRepost)
        return -1;
    else if (sNextRepost < vuxGameTime or reply not_eq DIVERT_NO_DIVERT)
    {
        Flight flight = (Flight) vuDatabase->Find(sDivertFlight);
        CampEntity target = NULL;

        // Clear repost time
        sNextRepost = 0;

        if ( not flight)
            return 0;

        // Check for target viability
        target = (CampEntity)vuDatabase->Find(sLastDivert.dataBlock.targetID);

        if ( not target or (target->IsUnit() and (((Unit)target)->IsDead() or ((Unit)target)->Broken())))
            return 0;

        if (flight not_eq FalconLocalSession->GetPlayerFlight())
        {
            sLastReply = DIVERT_REPLY_YES;
            ApplyDivert(flight, &sLastDivert);
            return 1;
        }
        else if (reply == DIVERT_REPLY_YES)
        {
            sLastReply = DIVERT_REPLY_YES;
            //ApplyDivert (flight, &sLastDivert); this is only for AI flights
            return 1;
        }
        else if (reply == DIVERT_REPLY_NO)
        {
            // KCK: Should we make some sort of enty in the mission evaluator that this divert was refused?
            // KCL: Should AWACs cuss us out for refusing?
            sLastReply = DIVERT_REPLY_NO;
            return 1;
        }
        else
        {
            // Make radio call and repost
            PlayDivertRadioCalls(target, sLastDivert.dataBlock.mission, flight, TRUE);
            sNextRepost = vuxGameTime + RESEND_DIVERT_TIME * CampaignSeconds;
            // TJL 12/21/03 No need to keep setting Divert 0 when asking for a response to a divert call
            //flight->SetDiverted(0); // Unset diverted to allow re-diverts

        }
    }

    return 0;
}

void ApplyDivert(Flight flight, FalconDivertMessage *fdm)
{
    int oldmission;

    oldmission = flight->GetUnitMission();

    if (flight->IsLocal())
    {
        // Build the divert mission (only for local entity)
        MissionRequestClass mis;

        mis.tot = fdm->dataBlock.tot;
        mis.mission = fdm->dataBlock.mission;
        mis.targetID = fdm->dataBlock.targetID;
        mis.requesterID = fdm->dataBlock.requesterID;
        mis.flags = fdm->dataBlock.flags;
        mis.tx = fdm->dataBlock.tx;
        mis.ty = fdm->dataBlock.ty;
        mis.priority = fdm->dataBlock.priority;

        if (mis.mission == AMIS_INTERCEPT)
            mis.context = interceptEnemyAircraft;
        else if (mis.mission == AMIS_CAS)
            mis.context = attackEnemyUnit;

        // Trying to track down a potential bug here.. It's hard enough to
        // get diverts I figure I'll let QA do the testing..
        ShiAssert(flight not_eq FalconLocalSession->GetPlayerFlight());

        flight->BuildMission(&mis);
    }

    // Generate a scramble message dialog box if this is an intercept divert on one of the
    // player squadron's alert missions.
    if (doUI and oldmission == AMIS_ALERT and fdm->dataBlock.mission == AMIS_INTERCEPT and flight->GetUnitSquadronID() == FalconLocalSession->GetPlayerSquadronID())
    {
        gInterceptersId = flight->Id();
        PostMessage(FalconDisplay.appWin, FM_ATTACK_WARNING, 0, 0);
    }
}


void PlayDivertRadioCalls(CampEntity target, int mission, Flight flight, int broadcast)
{
    FalconRadioChatterMessage *msg;
    VuTargetEntity *to;
    short newTarget = 1;
    GridIndex x = 0, y = 0;

    if (broadcast)
        to = FalconLocalGame;
    else
        to = FalconLocalSession;

    if (target)
    {
        // 2002-01-04 M.N. this is BS - we want to know the position of the target, not of an imaginary "Collision point"
        // Furthermore the FindCollisionPoint function in 2D seems to be rather erroreous (600 miles distance calculations...)
        /* vector collPoint;
         if(flight)
         {
         flight->FindCollisionPoint(target,&collPoint,TRUE);
         x = SimToGrid(collPoint.y);
         y = SimToGrid(collPoint.x);
         }
         else
         {*/
        x = SimToGrid(target->YPos());
        y = SimToGrid(target->XPos());

        // }
        if (flight->GetAssignedTarget() == target->Id())
            newTarget = 0;
    }

    if (mission <= 0 or not target)
    {
        if (mission == DIVERT_DENIGNED)
            SendCallFromAwacs(flight, rcNOTASKING, to); // Divert denigned
        else if (mission == DIVERT_CANCLED)
            SendCallFromAwacs(flight, rcENDDIVERTDIRECTIVE, to); // Divert canceled
        else if (mission == DIVERT_SUCCEEDED)
            SendCallFromAwacs(flight, rcENDDIVERTDIRECTIVE, to); // Divert over

        return;
    }
    else if ( not newTarget)
    {
        // Just a position update
        if (target->GetDomain() == DOMAIN_AIR)
        {
            GridIndex fx, fy;
            flight->GetLocation(&fx, &fy);

            if (DistSqu(x, y, fx, fy) < 25)
            {
                // Less than 5 km, call a mergeplot
                msg = CreateCallFromAwacs(flight, rcMERGEPLOT, to);
                msg->dataBlock.edata[4] = (short)((target->Type() - VU_LAST_ENTITY_TYPE) * 2);
            }
            else
            {
                // Otherwise vector
                msg = CreateCallFromAwacs(flight, rcAIRTARGETBRA, to);
                msg->dataBlock.edata[4] = x;
                msg->dataBlock.edata[5] = y;
                msg->dataBlock.edata[6] = (short)((Unit)target)->GetUnitAltitude();
            }
        }
        else
        {
            // Ground target
            msg = CreateCallFromAwacs(flight, rcGNDTARGETBR, to);
            msg->dataBlock.edata[4] = x;
            msg->dataBlock.edata[5] = y;
        }

        FalconSendMessage(msg, FALSE);
        return;
    }
    else if (mission == AMIS_INTERCEPT)
    {
        ShiAssert(target->IsFlight());
        GridIndex fx, fy;
        flight->GetLocation(&fx, &fy);
        msg = CreateCallFromAwacs(flight, rcENGAGEDIRECTIVE, to);
        msg->dataBlock.edata[4] = (short)((((Unit)target)->GetUnitClassData()->VehicleType[0]) * 2);
        msg->dataBlock.edata[5] = (short)((Unit)target)->GetTotalVehicles();
        msg->dataBlock.edata[6] = x;
        msg->dataBlock.edata[7] = y;

        if (target->IsFlight())
            msg->dataBlock.edata[8] = (short)(((Unit)target)->GetUnitAltitude() / 1000);
        else
            msg->dataBlock.edata[8] = 0;

        FalconSendMessage(msg, FALSE);
    }
    else if (mission == AMIS_CAS)
    {
        msg = CreateCallFromAwacs(flight, rcATTACKMYTARGET, to);

        if (target->IsUnit() and ((Unit)target)->GetUnitFormation() == GFORM_COLUMN)
            msg->dataBlock.edata[4] = (short)((target->Type() - VU_LAST_ENTITY_TYPE) * 6 + 1); // "X column"
        else
            msg->dataBlock.edata[4] = (short)((target->Type() - VU_LAST_ENTITY_TYPE) * 6 + 2); // "X unit"

        msg->dataBlock.edata[5] = x;
        msg->dataBlock.edata[6] = y;
        FalconSendMessage(msg, FALSE);
    }
    else if (mission == AMIS_ALERT)
    {
        msg = CreateCallFromAwacs(flight, rcSCRAMBLE, to);
        msg->dataBlock.edata[4] = x;
        msg->dataBlock.edata[5] = y;

        if (target->IsFlight())
            msg->dataBlock.edata[6] = (short)(((Unit)target)->GetUnitAltitude() / 1000);
        else
            msg->dataBlock.edata[6] = 0;

        FalconSendMessage(msg, FALSE);
    }
    else
    {
        msg = CreateCallFromAwacs(flight, rcGNDTARGETBR, to);
        msg->dataBlock.edata[4] = x;
        msg->dataBlock.edata[5] = y;
        FalconSendMessage(msg, FALSE);
    }

    // This is the flight saying that they're diverting (should be delayed a little..)
    if (SimDriver.GetPlayerEntity() and flight->GetComponentLead() not_eq SimDriver.GetPlayerEntity())
    {
        msg = CreateCallToAWACS(flight, rcAWACSDIVERT, to);
        msg->dataBlock.edata[0] = msg->dataBlock.edata[2];
        msg->dataBlock.edata[1] = msg->dataBlock.edata[3];
        msg->dataBlock.time_to_play = 2 * CampaignSeconds;
        FalconSendMessage(msg, FALSE);
    }
}


