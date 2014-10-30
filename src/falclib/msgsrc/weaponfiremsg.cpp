/*
 * Machine Generated source file for message "Weapon Fire".
 * NOTE: The functions here must be completed by hand.
 * Generated on 20-February-1997 at 17:20:42
 * Generated from file EVENTS.XLS by Leon Rosenshein
 */

#include "MsgInc/WeaponFireMsg.h"
#include "MsgInc/RadioChatterMsg.h"
#include "airunit.h"
#include "mesg.h"
#include "mesg.h"
#include "simbase.h"
#include "sms.h"
#include "simveh.h"
#include "team.h"
#include "MissEval.h"
#include "Cmpclass.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"

extern int gRebuildBubbleNow;

FalconWeaponsFire::FalconWeaponsFire(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent(WeaponFireMsg, FalconEvent::SimThread, entityId, target, loopback)
{
    // RequestReliableTransmit ();
    // RequestOutOfBandTransmit ();
    // Your Code Goes Here
}

FalconWeaponsFire::FalconWeaponsFire(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent(WeaponFireMsg, FalconEvent::SimThread, senderid, target)
{
    // Your Code Goes Here
    type;
}

FalconWeaponsFire::~FalconWeaponsFire(void)
{
    // Your Code Goes Here
}
extern bool g_bLogEvents;
int FalconWeaponsFire::Process(uchar autodisp)
{
    FalconEntity* theEntity;
    FalconEntity* theTarget = NULL;
    SimBaseClass* simEntity = NULL;

    if (autodisp)
        return 0;

    theTarget = (FalconEntity*) vuDatabase->Find(dataBlock.targetId);
    theEntity = (FalconEntity*)(vuDatabase->Find(dataBlock.fEntityID));

    if (theEntity)
    {
        if (theEntity->IsSim())
        {
            simEntity = (SimBaseClass*) theEntity;

            if (simEntity and not simEntity->IsLocal() and dataBlock.weaponType == GUN)
            {
                if (simEntity->nonLocalData)
                {
                    simEntity->nonLocalData->dx = dataBlock.dx;
                    simEntity->nonLocalData->dy = dataBlock.dy;
                    simEntity->nonLocalData->dz = dataBlock.dz;
                    simEntity->nonLocalData->flags or_eq NONLOCAL_GUNS_FIRING;
                    simEntity->nonLocalData->timer2 = 0;
                }

                if (dataBlock.fWeaponUID.num_ == 0) simEntity->SetFiring(FALSE);
                else simEntity->SetFiring(TRUE);
            }

            //Cobra test
            if ((theEntity->IsAirplane() or theEntity->IsHelicopter()) /* and dataBlock.weaponType not_eq FalconWeaponsFire::GUN*/)
            {
                FalconRadioChatterMessage *radioMessage = new FalconRadioChatterMessage(simEntity->Id(), FalconLocalSession);
                radioMessage->dataBlock.to = MESSAGE_FOR_TEAM;
                radioMessage->dataBlock.from = theEntity->Id();
                // radioMessage->dataBlock.voice_id = (uchar)((SimBaseClass*)theEntity)->GetPilotVoiceId();
                radioMessage->dataBlock.voice_id = ((FlightClass*)(((AircraftClass*)theEntity)->GetCampaignObject()))->GetPilotVoiceID(((AircraftClass*)theEntity)->GetCampaignObject()->GetComponentIndex(((AircraftClass*)theEntity)));

                // JPO - special case AMRAAM call
                // 2001-09-16 M.N. CHANGED FROM FalconWeaponsFire::MRM TO CT ID CHECK -> Aim-7 IS ALSO A MRM, BUT HAS A FOX-1 CALL
                if (WeaponDataTable[GetWeaponIdFromDescriptionIndex(dataBlock.fWeaponID - VU_LAST_ENTITY_TYPE)].Index == 227 or
                    WeaponDataTable[GetWeaponIdFromDescriptionIndex(dataBlock.fWeaponID - VU_LAST_ENTITY_TYPE)].Index == 228)
                {
                    radioMessage->dataBlock.message = rcFIREAMRAAM;
                    radioMessage->dataBlock.edata[0] = ((SimMoverClass*)theEntity)->vehicleInUnit;

                    if (theTarget)   // we have a target - how far away?
                    {
                        float dx = theTarget->XPos() - theEntity->XPos();
                        float dy = theTarget->YPos() - theEntity->YPos();
                        float dz = theTarget->ZPos() - theEntity->ZPos();

                        float distance = (float)sqrt(dx * dx + dy * dy + dz * dz) * FT_TO_NM;

                        if (distance < 5)
                            radioMessage->dataBlock.edata[1] = 1; // short
                        else if (distance > 15)
                            radioMessage->dataBlock.edata[1] = 3; // long
                        else
                            radioMessage->dataBlock.edata[1] = 2; // medium
                    }
                    else radioMessage->dataBlock.edata[1] = 0; // maddog
                }
                else
                {
                    radioMessage->dataBlock.message = rcFIRING;

                    //Total Fucking HACK
                    if (dataBlock.weaponType == FalconWeaponsFire::Rocket)
                        radioMessage->dataBlock.edata[0] = 887; // 2001-09-16 M.N. "Rockets" EVAL INDEX = 887, NOT 163
                    else
                        radioMessage->dataBlock.edata[0] = WeaponDataTable[GetWeaponIdFromDescriptionIndex(dataBlock.fWeaponID - VU_LAST_ENTITY_TYPE)].Index;
                }

                FalconSendMessage(radioMessage, FALSE);
            }

            //if theEntity shot at someone on their own team complain
            //edg: there seems to be a crash here if the target is NOT a SIm
            // GetPilotVoiceId is a SimBaseClass member.
            // check for Sim.  I'm not sure what needs to be done if its a camp entity
            //Cobra added weaponType check so bombs don't fall through into this section
            if (theTarget /* and theTarget->IsSim()*/ and dataBlock.weaponType < 3
               and (theTarget->IsAirplane() or theTarget->IsHelicopter()) and (GetTTRelations(theTarget->GetTeam(), simEntity->GetTeam()) < Hostile))
            {
                float playMessage = TRUE;

                if (dataBlock.weaponType == GUN)
                {
                    float az, el;

                    // Check rel geom
                    CalcRelAzEl(simEntity, theTarget->XPos(), theTarget->YPos(), theTarget->ZPos(), &az, &el);

                    if (fabs(az) > 10.0F * DTR or fabs(el) > 10.0F * DTR)
                        playMessage = FALSE;
                }

                if (playMessage and theTarget)
                {
                    FalconRadioChatterMessage *radioMessage = new FalconRadioChatterMessage(simEntity->Id(), FalconLocalSession);
                    radioMessage->dataBlock.to = MESSAGE_FOR_TEAM;
                    radioMessage->dataBlock.from = theTarget->Id();

                    if ( not ((AircraftClass*)theTarget)->GetCampaignObject())
                        radioMessage->dataBlock.voice_id = (uchar)((SimBaseClass*)theTarget)->GetPilotVoiceId();
                    else
                        radioMessage->dataBlock.voice_id = ((FlightClass*)(((AircraftClass*)theTarget)->GetCampaignObject()))->GetPilotVoiceID(((AircraftClass*)theTarget)->GetCampaignObject()->GetComponentIndex(((AircraftClass*)theTarget)));

                    radioMessage->dataBlock.message = rcSHOTWINGMAN;
                    radioMessage->dataBlock.edata[0] = 32767;
                    FalconSendMessage(radioMessage, FALSE);
                }
            }

            // Register the shot if it was by someone in our package me123 or we are server
            if (dataBlock.fWeaponUID not_eq FalconNullId and simEntity->GetCampaignObject() and ((simEntity->GetCampaignObject())->InPackage() or FalconLocalGame->GetGameType() == game_InstantAction or g_bLogEvents))
                TheCampaign.MissionEvaluator->RegisterShot(this);

        }
        else
        {
            // Register the shot if it was by someone in our package
            if (dataBlock.fWeaponUID not_eq FalconNullId and theEntity->IsCampaign() and (((CampEntity)theEntity)->InPackage() or FalconLocalGame->GetGameType() == game_InstantAction or g_bLogEvents))
                TheCampaign.MissionEvaluator->RegisterShot(this);
        }
    }

    // Register the shot if it was at the player
    if (TheCampaign.MissionEvaluator and dataBlock.targetId == FalconLocalSession->GetPlayerEntityID() or g_bLogEvents)
        if (theTarget and theTarget->IsSetFalcFlag(FEC_HASPLAYERS) and theTarget->IsSim() and ((SimBaseClass*)theTarget)->GetCampaignObject())
            TheCampaign.MissionEvaluator->RegisterShotAtPlayer(this, ((SimBaseClass*)theTarget)->GetCampaignObject()->GetCampID(), ((SimMoverClass*)theTarget)->pilotSlot); //me123 added

    //else TheCampaign.MissionEvaluator->RegisterShotAtPlayer(this,NULL,NULL);//me123 maddog

    // if the weapon is a missile and we have a target ID, tell the target
    // there's an incoming
    if (dataBlock.targetId not_eq vuNullId and 
        (dataBlock.weaponType == SRM or dataBlock.weaponType == MRM))
    {
        SimBaseClass *theMissile = (SimBaseClass*)(vuDatabase->Find(dataBlock.fWeaponUID));

        if (theTarget and theMissile and theTarget->IsSim())
        {
            ((SimBaseClass*)theTarget)->SetIncomingMissile(theMissile);

            if (simEntity and not simEntity->OnGround())
                ((SimBaseClass*)theTarget)->SetThreat(simEntity, THREAT_MISSILE);
        }
    }
    else if (dataBlock.targetId not_eq vuNullId and dataBlock.weaponType == GUN)
    {
        if (theTarget and theTarget->IsSim())
        {
            if (simEntity and not simEntity->OnGround())
                ((SimBaseClass*)theTarget)->SetThreat(simEntity, THREAT_GUN);
        }
    }

    // WM 11-12-03  Fix the annoying missile bug where you can't see other client's missiles if
    //   the host doesn't join the 3D world.
    if (dataBlock.weaponType == SRM or dataBlock.weaponType == MRM  or dataBlock.weaponType == AGM or dataBlock.weaponType == BMB)
    {
        gRebuildBubbleNow = 2;
    }

    return TRUE;
}

