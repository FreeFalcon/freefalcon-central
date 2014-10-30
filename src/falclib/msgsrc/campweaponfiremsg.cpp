/*
 * Machine Generated source file for message "Campaign Weap Fire".
 * NOTE: The functions here must be completed by hand.
 * Generated on 18-March-1998 at 09:20:55
 * Generated from file EVENTS.XLS by MicroProse
 */

#include "MsgInc/CampWeaponFireMsg.h"
#include "MsgInc/WeaponFireMsg.h"
#include "MsgInc/RadioChatterMsg.h"
#include "MsgInc/MissileEndMsg.h"
#include "MsgInc/TrackMsg.h"
#include "MsgInc/DamageMsg.h"
#include "MissEval.h"
#include "flight.h"
#include "mesg.h"
#include "Unit.h"
#include "Objectiv.h"
#include "ClassTbl.h"
#include "Team.h"
#include "Sfx.h"
#include "OtwDrive.h"
#include "Dispcfg.h"
#include "FalcUser.h"
#include "simbase.h"
#include "Graphics/include/rviewpnt.h"
#include "fakerand.h"
#include "missile.h"
#include "object.h"
#include "misslist.h"
#include "initData.h"
#include "BeamRider.h"
#include "simdrive.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"
#include "InvalidBufferException.h"
#include "Graphics/Include/drawparticlesys.h"

#define BANDIT_VEH 2800 // 2002-02-21 S.G.

class C_Handler;

uchar gDamageStatusBuffer[256];
uchar *gDamageStatusPtr;

extern int gRebuildBubbleNow;

extern int InterestingSFX(float x, float y);
extern C_Handler *gMainHandler;

void DoDistanceVisualEffects(CampEntity shooter, CampEntity target, int weapon_id, int shots);
void DoShortDistanceVisualEffects(CampEntity shooter, CampEntity target, int weapon_id, int shots);
void FireMissileAtSim(CampEntity shooter, SimBaseClass *simTarg, short weapId);
void CreateDrawable(SimBaseClass *, float scale);
void FireOnSimEntity(CampEntity shooter, SimBaseClass *simTarg, short weaponId, int shots);
FalconDamageMessage * GetSimDamageMessage(CampEntity shooter, SimBaseClass *target, float rangeSq, int damageType, int weapId);

// ======================================
// The message stuff
// ======================================

FalconCampWeaponsFire::FalconCampWeaponsFire(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent(CampWeaponFireMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{
    dataBlock.data = NULL;
    dataBlock.fWeaponUID = FalconNullId;
    dataBlock.fPilotId = 255;
    dataBlock.dPilotId = 255;
    dataBlock.size = 0;
}

FalconCampWeaponsFire::FalconCampWeaponsFire(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent(CampWeaponFireMsg, FalconEvent::CampaignThread, senderid, target)
{
    dataBlock.data = NULL;
    dataBlock.fWeaponUID = FalconNullId;
    dataBlock.fPilotId = 255;
    dataBlock.dPilotId = 255;
    dataBlock.size = 0;
    type;
}

FalconCampWeaponsFire::~FalconCampWeaponsFire(void)
{
    if (dataBlock.data)
        delete dataBlock.data;
}

int FalconCampWeaponsFire::Size() const
{
    ShiAssert(dataBlock.size >= 0);
    return FalconEvent::Size() +
            sizeof(VU_ID) + sizeof(VU_ID) + sizeof(dataBlock.weapon) +
            sizeof(dataBlock.shots) + sizeof(uchar) + sizeof(uchar) + sizeof(ushort) + dataBlock.size
            ;
}

//sfr: changed to long *
int FalconCampWeaponsFire::Decode(VU_BYTE **buf, long *rem)
{
    long init = *rem;

    memcpychk(&dataBlock.shooterID, buf, sizeof(VU_ID), rem);
    memcpychk(&dataBlock.fWeaponUID, buf, sizeof(VU_ID), rem);
    memcpychk(dataBlock.weapon, buf, sizeof dataBlock.weapon, rem);
    memcpychk(dataBlock.shots, buf, sizeof dataBlock.shots, rem);
    memcpychk(&dataBlock.fPilotId, buf, sizeof(uchar), rem);
    memcpychk(&dataBlock.dPilotId, buf, sizeof(uchar), rem);
    memcpychk(&dataBlock.size, buf, sizeof(ushort), rem);

    //ShiAssert ( dataBlock.size >= 0 );
    if (dataBlock.size)
    {
        dataBlock.data = new uchar[dataBlock.size];
        memcpychk(dataBlock.data, buf, dataBlock.size, rem);
    }

    FalconEvent::Decode(buf, rem);

    int size = init - *rem;
    ShiAssert(size == Size());
    return size;
}

int FalconCampWeaponsFire::Encode(VU_BYTE **buf)
{
    int size = 0;

    ShiAssert(dataBlock.size >= 0);
    memcpy(*buf, &dataBlock.shooterID, sizeof(VU_ID));
    *buf += sizeof(VU_ID);
    size += sizeof(VU_ID);
    memcpy(*buf, &dataBlock.fWeaponUID, sizeof(VU_ID));
    *buf += sizeof(VU_ID);
    size += sizeof(VU_ID);
    memcpy(*buf, dataBlock.weapon, sizeof dataBlock.weapon);
    *buf += sizeof dataBlock.weapon;
    size += sizeof dataBlock.weapon;
    memcpy(*buf, dataBlock.shots, sizeof dataBlock.shots);
    *buf += sizeof dataBlock.shots;
    size += sizeof dataBlock.shots;
    memcpy(*buf, &dataBlock.fPilotId, sizeof(uchar));
    *buf += sizeof(uchar);
    size += sizeof(uchar);
    memcpy(*buf, &dataBlock.dPilotId, sizeof(uchar));
    *buf += sizeof(uchar);
    size += sizeof(uchar);
    memcpy(*buf, &dataBlock.size, sizeof(ushort));
    *buf += sizeof(ushort);
    size += sizeof(ushort);

    if (dataBlock.size)
    {
        memcpy(*buf, dataBlock.data, dataBlock.size);
        *buf += dataBlock.size;
        size += dataBlock.size;
    }

    size += FalconEvent::Encode(buf);

    ShiAssert(size == Size());

    return size;
}
extern bool g_bLogEvents;
int FalconCampWeaponsFire::Process(uchar autodisp)
{
    CampEntity target = (CampEntity)vuDatabase->Find(EntityId());
    CampEntity shooter = (CampEntity)vuDatabase->Find(dataBlock.shooterID);
    int losses, shooterAc = 255, i;
    FalconDeathMessage *dtm = NULL;
    //sfr: again we cant use datablock directly
    VU_BYTE *data = dataBlock.data;

    if (autodisp or not shooter or not target or not shooter->IsUnit())
        return -1;

    if ( not target->IsAggregate())
    {
        // Whoops, this thing deaggregated out from under us.
        // If we're the host, actually start firing the stuff.
        if (shooter->IsLocal())
            FireOnSimEntity(shooter, target, dataBlock.weapon, dataBlock.shots, dataBlock.dPilotId);

        return 0;
    }

    shooter->ReturnToSearch();

    if (shooter->IsFlight())
    {
        // Pick a pilot to get the kill
        if (dataBlock.fPilotId == 255)
            shooterAc = dataBlock.fPilotId = (uchar)((Flight)shooter)->PickRandomPilot(target->Id().num_);

        if (dataBlock.fPilotId >= PILOTS_PER_FLIGHT)
            shooterAc = ((Flight)shooter)->GetAdjustedPlayerSlot(dataBlock.fPilotId);
        else
            shooterAc = dataBlock.fPilotId;
    }
    else
    {
        shooterAc = 0; // something
    }

    if (shooter->IsAggregate())
    {
        // Send a radio chatter message to LOCAL MACHINE if shooter is a flight
        if (shooter->IsFlight() and not SimDriver.InSim() and not (rand() % 20))
        {
            // Send the chatter message;
            FalconRadioChatterMessage *msg = new FalconRadioChatterMessage(shooter->Id(), FalconLocalSession);
            msg->dataBlock.from = shooter->Id();
            msg->dataBlock.to = MESSAGE_FOR_TEAM;
            msg->dataBlock.voice_id = (uchar)((Flight)shooter)->GetPilotVoiceID(shooterAc);

            if (target->IsFlight())
            {
                msg->dataBlock.message = rcFIRING;
                // JWFU: Need callsign data stuff
                msg->dataBlock.edata[0] = WeaponDataTable[dataBlock.weapon[0]].Index;
            }
            else
            {
                msg->dataBlock.message = rcATTACKINGA;
                msg->dataBlock.edata[0] = ((Flight)shooter)->callsign_id;
                msg->dataBlock.edata[1] = (short)((Flight)shooter)->GetPilotCallNumber(shooterAc);
                target->GetLocation(&msg->dataBlock.edata[2], &msg->dataBlock.edata[3]);
            }

            FalconSendMessage(msg, FALSE);
        }

        // Synthisize a shot message for flight shooters in our package
        if (shooter->IsFlight() and (shooter->InPackage() or g_bLogEvents))
        {
            FalconWeaponsFire wfm(FalconNullId, FalconLocalSession);
            wfm.dataBlock.fCampID = shooter->GetCampID();
            wfm.dataBlock.fPilotID = dataBlock.fPilotId;
            wfm.dataBlock.fIndex = (unsigned short)(((Unit)shooter)->GetVehicleID(0) + VU_LAST_ENTITY_TYPE);
            wfm.dataBlock.fSide = (unsigned char)shooter->GetOwner();
            wfm.dataBlock.fWeaponID = (unsigned short)(WeaponDataTable[dataBlock.weapon[0]].Index + VU_LAST_ENTITY_TYPE);
            // KCK: Since we don't really have a real weapon, use the current time for matching
            wfm.dataBlock.fWeaponUID.num_ = dataBlock.fWeaponUID.num_ = TheCampaign.CurrentTime;
            TheCampaign.MissionEvaluator->RegisterShot(&wfm);
        }

        // Do visual Effects (Aggregate shooters only)
        if (InterestingSFX(target->XPos(), target->YPos()))
        {
            for (i = 0; i < MAX_TYPES_PER_CAMP_FIRE_MESSAGE and dataBlock.weapon[i] and dataBlock.shots[i]; i++)
                DoDistanceVisualEffects(shooter, target, dataBlock.weapon[i], dataBlock.shots[i]);
        }
    }

    // Synthisize a death message if either shooter or target is in our package
    if (shooter->InPackage() or target->InPackage())
    {
        dtm = new FalconDeathMessage(FalconNullId, FalconLocalSession);
        dtm->dataBlock.damageType = 0;

        if (target->IsFlight())
            dtm->dataBlock.dPilotID = (uchar)((Flight)target)->PickRandomPilot(target->GetCampID());

        dtm->dataBlock.dCampID = target->GetCampID();
        dtm->dataBlock.dSide = target->GetOwner();
        dtm->dataBlock.fCampID = shooter->GetCampID();
        dtm->dataBlock.fPilotID = dataBlock.fPilotId;
        dtm->dataBlock.fIndex = (unsigned short)(((Unit)shooter)->GetVehicleID(0) + VU_LAST_ENTITY_TYPE);
        dtm->dataBlock.fSide = shooter->GetOwner();
        dtm->dataBlock.fWeaponID = (unsigned short)(WeaponDataTable[dataBlock.weapon[0]].Index + VU_LAST_ENTITY_TYPE);
        dtm->dataBlock.fWeaponUID = dataBlock.fWeaponUID;
    }

    // Apply the damage data
    losses = target->DecodeDamageData(data, (Unit)shooter, dtm);

    // add some additional fire effects if losses were taken, the target is
    // a battalion and the target is in the sim lists
    if (losses and 
        target->InSimLists() and 
        OTWDriver.IsActive())
    {
        int i;
        Tpoint pos;
        //RV - I-Hawk - Added a 0 vector for RV new PS calls
        Tpoint PSvec;
        PSvec.x = 0;
        PSvec.y = 0;
        PSvec.z = 0;

        // ground losses
        if (target->IsBattalion() or target->IsObjective())
        {
            pos.z = 40.0f;

            for (i = 0; i < losses; i++)
            {
                /*RV - I-Hawk - Removed VEHICLE_BURNING here,
                  It's called per vehicle at simveh.cpp
                pos.x = target->XPos() + 800.0f * PRANDFloat();
                pos.y = target->YPos() + 800.0f * PRANDFloat();
                OTWDriver.AddSfxRequest( new SfxClass( SFX_VEHICLE_BURNING,
                 &pos,
                 60.0f,
                 90.0f ) );
                */

                pos.x = target->XPos() + 800.0f * PRANDFloat();
                pos.y = target->YPos() + 800.0f * PRANDFloat();
                /*
                OTWDriver.AddSfxRequest( new SfxClass( SFX_CAMP_HIT_EXPLOSION_DEBRISTRAIL,
                 &pos,
                 2.0f,
                 200.0f ) );
                */
                DrawableParticleSys::PS_AddParticleEx((SFX_CAMP_HIT_EXPLOSION_DEBRISTRAIL + 1),
                                                      &pos,
                                                      &PSvec);
            }
        }
        // air losses
        else
        {

            for (i = 0; i < losses; i++)
            {
                pos.x = target->XPos() + 800.0f * PRANDFloat();
                pos.y = target->YPos() + 800.0f * PRANDFloat();
                pos.z = target->ZPos() + 300.0f * PRANDFloat();
                /*
                OTWDriver.AddSfxRequest( new SfxClass( SFX_CAMP_HIT_EXPLOSION_DEBRISTRAIL,
                 &pos,
                 2.0f,
                 200.0f ) );
                */
                DrawableParticleSys::PS_AddParticleEx((SFX_CAMP_HIT_EXPLOSION_DEBRISTRAIL + 1),
                                                      &pos,
                                                      &PSvec);
            }
        }

    }

    if (dtm)
        delete dtm;

    // Send a RadioChatter message to LOCAL MACHINE if shooter is a flight and scored a kill
    if (losses and shooter->IsFlight())
    {
        FalconRadioChatterMessage *msg = new FalconRadioChatterMessage(target->Id(), FalconLocalSession);
        msg->dataBlock.from = shooter->Id();
        msg->dataBlock.to = MESSAGE_FOR_TEAM;
        msg->dataBlock.voice_id = (uchar)((Flight)shooter)->GetPilotVoiceID(shooterAc);

        if (target->IsFlight())
        {
            msg->dataBlock.message = rcAIRBDA;
            msg->dataBlock.edata[0] = ((Flight)shooter)->callsign_id;
            msg->dataBlock.edata[1] = (short)((Flight)shooter)->GetPilotCallNumber(shooterAc);
            //MI uncommented the line below and outcommented the lines
            //M.N. changed to 32767 which flexibly uses randomized values of available eval indexes
            msg->dataBlock.edata[2] = 32767; // couldn't stand the Hollywood kill calls
            /*if(rand()%2)
              msg->dataBlock.edata[2] = 1;
              else
              msg->dataBlock.edata[2] = 9;*/
        }
        else if (target->IsTaskForce())
        {
            msg->dataBlock.message = rcMOVERBDA;
            msg->dataBlock.edata[0] = 32767;
        }
        else
        {
            if (target->IsUnit())
            {
                msg->dataBlock.message = rcMOVERBDA;
                msg->dataBlock.edata[0] = 32767;
            }
            else
            {
                msg->dataBlock.message = rcSTATICBDA;
                msg->dataBlock.edata[0] = ((Flight)shooter)->callsign_id;
                msg->dataBlock.edata[1] = (short)((Flight)shooter)->GetPilotCallNumber(shooterAc);
                msg->dataBlock.edata[2] = 32767;
            }
        }

        FalconSendMessage(msg, FALSE);
    }

    // COBRA - RED - SUSPENDED FOR NOW, caused a CTD
    //Cobra TJL Let's say we missed ;)
    /*else
     {
     FalconRadioChatterMessage *msg = new FalconRadioChatterMessage(target->Id(), FalconLocalSession);
     msg->dataBlock.from = shooter->Id();
     msg->dataBlock.to = MESSAGE_FOR_TEAM;
     msg->dataBlock.voice_id = (uchar)((Flight)shooter)->GetPilotVoiceID(shooterAc);
     msg->dataBlock.message = rcMISSED;
     msg->dataBlock.edata[0] = 32767;
     FalconSendMessage(msg, FALSE);
     }*/

    // Send a CampEvent message for weapon fire to the LOCAL MACHINE
    FalconCampEventMessage *newEvent = new FalconCampEventMessage(shooter->Id(), FalconLocalSession);
    newEvent->dataBlock.flags = 0;
    newEvent->dataBlock.team = shooter->GetTeam();

    if (shooter->GetDomain() == DOMAIN_AIR)
    {
        if (target->IsObjective())
            newEvent->dataBlock.eventType = FalconCampEventMessage::campStrike;
        else if (target->GetDomain() == DOMAIN_AIR)
            newEvent->dataBlock.eventType = FalconCampEventMessage::campAirCombat;
        else if (target->GetDomain() == DOMAIN_LAND)
            newEvent->dataBlock.eventType = FalconCampEventMessage::campGroundAttack;
        else
            newEvent->dataBlock.eventType = FalconCampEventMessage::campCombat;
    }
    else
        newEvent->dataBlock.eventType = FalconCampEventMessage::campCombat;

    newEvent->dataBlock.data.vuIds[0] = shooter->Id();
    newEvent->dataBlock.data.vuIds[1] = target->Id();
    newEvent->dataBlock.data.owners[0] = shooter->GetOwner();
    newEvent->dataBlock.data.owners[1] = target->GetOwner();
    target->GetLocation(&newEvent->dataBlock.data.xLoc, &newEvent->dataBlock.data.yLoc);

    if (shooter->GetDomain() == DOMAIN_AIR)
    {
        // 2002-02-21 ADDED BY S.G. If it's not spotted and it's NOT the player, use the 'Bandit' vehicle so we don't warn the player on the identity of the shooter
        if ( not shooter->GetIdentified(target->GetTeam()) and FalconLocalSession->GetTeam() not_eq shooter->GetTeam())
            newEvent->dataBlock.data.textIds[0] = (short)(-1 * BANDIT_VEH);
        else
            // END OF ADDED SECTION 2002-02-21
            newEvent->dataBlock.data.textIds[0] = (short)(-1 * ((Unit)shooter)->GetVehicleID(0));

        if (target->IsObjective())
        {
            // Air Strikes
            newEvent->dataBlock.data.formatId = 1804;
        }
        else if (target->GetDomain() == DOMAIN_AIR)
        {
            // Air to air combat
            newEvent->dataBlock.data.formatId = 1805;

            // 2002-02-21 ADDED BY S.G. If it's not spotted and it's NOT the player, use the 'Bandit' vehicle so we don't warn the player on the identity of the shooter
            if ( not target->GetIdentified(shooter->GetTeam()) and FalconLocalSession->GetTeam() not_eq target->GetTeam())
                newEvent->dataBlock.data.textIds[1] = (short)(-1 * BANDIT_VEH);
            else
                // END OF ADDED SECTION 2002-02-21
                newEvent->dataBlock.data.textIds[1] = (short)(-1 * ((Unit)target)->GetVehicleID(0));
        }
        else
        {
            // Air attack
            newEvent->dataBlock.data.formatId = 1803;
        }
    }
    else if (shooter->GetDomain() == DOMAIN_SEA)
    {
        // Naval engagement
        newEvent->dataBlock.data.formatId = 1802;
    }
    else
    {
        if (target->IsObjective())
        {
            // Ground assault
            newEvent->dataBlock.data.formatId = 1806;
        }
        else if (target->GetDomain() == DOMAIN_AIR)
        {
            // Air defenses firing
            newEvent->dataBlock.data.formatId = 1801;
        }
        else
        {
            // Ground engagement (artillery or regular)
            if (shooter->GetSType() == STYPE_UNIT_ROCKET or shooter->GetSType() == STYPE_UNIT_SP_ARTILLERY or shooter->GetSType() == STYPE_UNIT_TOWED_ARTILLERY)
                newEvent->dataBlock.data.formatId = 1807;
            else
                newEvent->dataBlock.data.formatId = 1800;
        }
    }

    SendCampUIMessage(newEvent);

    // Send a CampEvent message for losses to the LOCAL MACHINE
    if (target->IsFlight() and losses)
    {
        FalconCampEventMessage *newEvent = new FalconCampEventMessage(target->Id(), FalconLocalGame);
        newEvent->dataBlock.team = GetEnemyTeam(target->GetTeam());
        newEvent->dataBlock.eventType = FalconCampEventMessage::campLosses;
        target->GetLocation(&newEvent->dataBlock.data.xLoc, &newEvent->dataBlock.data.yLoc);
        newEvent->dataBlock.data.formatId = 1825;

        // 2002-02-21 ADDED BY S.G. If it's not spotted and it's NOT the player, use the 'Bandit' vehicle so we don't warn the player on the identity of the shooter
        if ( not target->GetIdentified(shooter->GetTeam()) and FalconLocalSession->GetTeam() not_eq target->GetTeam())
            newEvent->dataBlock.data.textIds[0] = (short)(-1 * BANDIT_VEH);
        else
            // END OF ADDED SECTION 2002-02-21
            newEvent->dataBlock.data.textIds[0] = (short)(-1 * ((Unit)target)->GetVehicleID(0));

        newEvent->dataBlock.data.owners[0] = target->GetOwner();
        SendCampUIMessage(newEvent);
    }

    if (gMainHandler and FalconLocalSession->GetPlayerSquadron() and target->Id() == FalconLocalSession->GetPlayerSquadron()->GetUnitAirbaseID())
        PostMessage(FalconDisplay.appWin, FM_AIRBASE_ATTACK, 0, 0);

    return 0;
}


/*
 ** Name: GetSimTarget
 ** Description:
 ** Given a campenitity target, pull out one of the sim objects
 ** and return it as an object to target for damaging....
 */
SimBaseClass* GetSimTarget(CampEntity target, uchar targetId)
{
    SimBaseClass *theObj = NULL;
    int comp = 0, i = 0;

    if (target->GetComponents() == NULL)
        return NULL;

    // Get a specific component
    if (targetId not_eq 255)
        theObj = target->GetComponentNumber(targetId);

    if ( not theObj)
    {
        // Fire at a random unit component, if campaign unit is deaggregated
        if (target->IsUnit())
            comp = ((Unit)target)->GetTotalVehicles();
        else if (target->IsObjective())
            comp = ((Objective)target)->GetTotalFeatures();

        if (comp)
        {
            i = rand() % comp;
            theObj = (target)->GetComponentEntity(i);
        }
    }

    return theObj;
}

/*
 ** Name: FireOnSimEntity
 ** Description:
 ** This function handles the case where a campaign entity fires on
 ** a deaggregated campaign entity
 */
void FireOnSimEntity(CampEntity shooter, CampEntity campTarg, short weapon[], uchar shots[], uchar targetId)
{
    SimBaseClass *simTarg;
    int i;

    // don't run if OTWdrive not active
    if ( not OTWDriver.IsActive())
        return;

    for (i = 0; i < shots[0]; i++)
    {
        // get a component sim base out of the deagg'd unit
        simTarg = GetSimTarget(campTarg, targetId);

        if (simTarg == NULL)
        {
            MonoPrint("Sim Target is NULL?\n");
            continue;
        }

        FireOnSimEntity(shooter, simTarg, weapon[0]);
    }
}

/*
 ** Name: FireOnSimEntity
 ** Description:
 ** This function handles the case where a campaign entity fires on
 ** a sim entity
 */
void FireOnSimEntity(CampEntity shooter, SimBaseClass *simTarg, short weaponId)
{
    WeaponClassDataType *wc;
    FalconMissileEndMessage *endMessage = NULL;
    FalconDamageMessage *damMessage = NULL;
    Falcon4EntityClassType *classPtr;
    float blastRange, rangeSquare;
    BOOL hitSomething;

    // get the weapon class pointer
    wc = (WeaponClassDataType *)Falcon4ClassTable[WeaponDataTable[weaponId].Index].dataPtr;
    classPtr = &Falcon4ClassTable[WeaponDataTable[weaponId].Index ];

    // MonoPrint("Campaign Unit firing on sim entity. Weapon ID: %d, Shots: %d.\n",weaponId,shots);

    blastRange = 0.0f;

    hitSomething = FALSE;

    // what have we got for a weapon?
    if (classPtr->vuClassData.classInfo_[VU_CLASS] == CLASS_WEAPON and 
        classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_GUN)
    {
        if (wc->Flags bitand WEAP_TRACER)
        {
            // don't handle tracers
            // MonoPrint("Campaign unit unable to fire on sim entity due to gun code not existing.\n");
            return;
        }

        // ok, it's a shell
        // MonoPrint( "Itsa Shell\n" );

        // damage stuff goes here....
        if (wc->BlastRadius == 0)
        {
            // must be a direct hit  1-8 fer now
            if ((rand() bitand 7) == 7)
            {
                hitSomething = TRUE;
                damMessage = GetSimDamageMessage(shooter,
                                                 simTarg,
                                                 0.0f,
                                                 FalconDamageType::MissileDamage,
                                                 WeaponDataTable[weaponId].Index
                                                );
            }
            else
            {
                blastRange = (float)wc->BlastRadius;
            }
        }
        else
        {
            blastRange = rangeSquare = (float)wc->BlastRadius;
            rangeSquare *= 1.5f * PRANDFloatPos();

            if (rangeSquare < blastRange)
            {
                hitSomething = TRUE;
                rangeSquare *= rangeSquare;
                damMessage = GetSimDamageMessage(shooter,
                                                 simTarg,
                                                 rangeSquare,
                                                 FalconDamageType::MissileDamage,
                                                 WeaponDataTable[weaponId].Index
                                                );
            }
        }

        // just creating visual effect for now.....
        // missile end message
        endMessage = new FalconMissileEndMessage(shooter->Id(), FalconLocalSession);
        endMessage->dataBlock.fEntityID  = shooter->Id();

        if (shooter->IsFlight())
            endMessage->dataBlock.fPilotID = (uchar)((Flight)shooter)->PickRandomPilot(simTarg->Id().num_);
        else
            endMessage->dataBlock.fPilotID   = 255;

        endMessage->dataBlock.fIndex     = shooter->Type();

        endMessage->dataBlock.fCampID = shooter->GetCampID();
        endMessage->dataBlock.fSide   = shooter->GetOwner();

        endMessage->dataBlock.dEntityID  = simTarg->Id();
        endMessage->dataBlock.dCampID = simTarg->GetCampaignObject()->GetCampID();
        endMessage->dataBlock.dSide   = simTarg->GetCampaignObject()->GetOwner();

        endMessage->dataBlock.dPilotID   = 0;
        endMessage->dataBlock.dCampSlot  = 0;
        endMessage->dataBlock.dIndex     = 0;
        endMessage->dataBlock.fWeaponUID = shooter->Id();
        endMessage->dataBlock.wIndex   = (unsigned short)(WeaponDataTable[weaponId].Index + VU_LAST_ENTITY_TYPE);

        if (hitSomething)
            endMessage->dataBlock.endCode    = FalconMissileEndMessage::MissileKill;
        else
            endMessage->dataBlock.endCode    = FalconMissileEndMessage::Missed;

        endMessage->dataBlock.xDelta    = 1500.0f;
        endMessage->dataBlock.yDelta    = 0.0f;
        endMessage->dataBlock.zDelta    = 0.0f;

        // if target is on the ground get ground level and type
        if (simTarg->OnGround())
        {
            endMessage->dataBlock.x    = simTarg->XPos() + blastRange * PRANDFloat();
            endMessage->dataBlock.y    = simTarg->YPos() + blastRange * PRANDFloat();
            endMessage->dataBlock.z    = simTarg->ZPos();

            endMessage->dataBlock.groundType    =
                (char)OTWDriver.GetGroundType(endMessage->dataBlock.x,
                                              endMessage->dataBlock.y);
        }
        else
        {
            // edg note: eventually we want to check damage type prior to
            // placing the end effect
            // effect is 2 secs out from target's current position
            endMessage->dataBlock.x    = simTarg->XPos() + simTarg->XDelta() * SimLibMajorFrameTime;
            endMessage->dataBlock.y    = simTarg->YPos() + simTarg->YDelta() * SimLibMajorFrameTime;
            endMessage->dataBlock.z    = simTarg->ZPos() + simTarg->ZDelta() * SimLibMajorFrameTime;
            endMessage->dataBlock.groundType    = -1;
        }

        // the special effects driver will space out the damage over
        // some random time
        OTWDriver.AddSfxRequest(new SfxClass(
                                    endMessage,
                                    damMessage));

    }
    // itsa missile
    else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_MISSILE)
    {
        FireMissileAtSim(shooter, simTarg, weaponId);
    }
    else
    {
        // do we need to handle any other types?
        // MonoPrint( "Not Weapon Class for Camp Weapon Message\n" );
        return;
    }
}

/*
 ** Name: GetSimDamageMessage
 ** Description:
 */
FalconDamageMessage *
GetSimDamageMessage(CampEntity shooter,
                    SimBaseClass *target,
                    float rangeSq,
                    int damageType,
                    int weapId)
{
    FalconDamageMessage *message;
    float lethalRadius;
    CampBaseClass *campTarg;

    campTarg = target->GetCampaignObject();
    message = new FalconDamageMessage(target->Id(), FalconLocalSession);
    message->dataBlock.fEntityID  = shooter->Id();
    message->dataBlock.fCampID = shooter->GetCampID();
    message->dataBlock.fSide   = shooter->GetOwner();

    if (shooter->IsFlight())
        message->dataBlock.fPilotID = (uchar)((Flight)shooter)->PickRandomPilot(campTarg->Id().num_);
    else
        message->dataBlock.fPilotID   = 255;

    message->dataBlock.fIndex     = shooter->Type();
    message->dataBlock.fWeaponID  = (unsigned short)(weapId + VU_LAST_ENTITY_TYPE);
    message->dataBlock.fWeaponUID = shooter->Id();

    message->dataBlock.dEntityID  = target->Id();
    message->dataBlock.dCampID = campTarg->GetCampID();
    message->dataBlock.dSide   = campTarg->GetOwner();
    message->dataBlock.dPilotID   = 255;
    message->dataBlock.dIndex     = target->Type();

    message->dataBlock.damageRandomFact = 1.0f;

    WeaponClassDataType* wc = (WeaponClassDataType *)Falcon4ClassTable[weapId].dataPtr;

    lethalRadius = (float)wc->BlastRadius * wc->BlastRadius;

    if (rangeSq < POINT_BLANK)
    {
        // Direct hit
        message->dataBlock.damageType = damageType;
    }
    else
    {
        // Adjust damage for distance:
        message->dataBlock.damageType = FalconDamageType::ProximityDamage;
        message->dataBlock.damageStrength = (lethalRadius - rangeSq) / lethalRadius * wc->Strength;
    }


    // Player setting damage modifier
    /*
       if (PlayerOptions.GetWeaponEffectiveness() == WEEnhanced)
       message->dataBlock.damageRandomFact += 2.0F;
       if (PlayerOptions.GetWeaponEffectiveness() == WEExaggerated)
       message->dataBlock.damageRandomFact += 4.0F;
     */

    // FalconSendMessage (message,FALSE);
    //me123 message->RequestOutOfBandTransmit ();
    return message;
}


// ============================
// Visual effects stuff
// ============================

void DoDistanceVisualEffects(CampEntity shooter, CampEntity target, int weapon_id, int shots)
{
    Tpoint  pos, tar, vec;
    int stype;
    float interval;
    // float d;
    VuEntity *player;
    float dx, dy;
    BOOL    shortDist = TRUE;

    //RV - I-Hawk - Added a 0 vector for RV new PS calls
    Tpoint PSvec;
    PSvec.x = 0;
    PSvec.y = 0;
    PSvec.z = 0;



    // make sure OTWDriver is ready
    if ( not OTWDriver.IsActive())
        return;

    player = FalconLocalSession->GetCameraEntity(0);

    // Find shooter position, position of target unit and the normalized vector to target
    pos.x = shooter->XPos();
    pos.y = shooter->YPos();
    pos.z = shooter->ZPos();
    tar.x = target->XPos();
    tar.y = target->YPos();
    tar.z = target->ZPos();
    /* KCK NOTE: We're actually not shooting at our target - just random directions
     */
    vec.x = tar.x - pos.x;
    vec.y = tar.y - pos.y;
    vec.z = tar.z - pos.z;

    /*
     ** edg: don't normalize vector.  We're going to try giving the effects
     ** better placement

     d = sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
     if ( d not_eq 0.0f )
     {
     vec.x /= d;
     vec.y /= d;
     vec.z /= d;
     }
     */

    if (player)
    {
        dx = (float)fabs(player->XPos() - tar.x);
        dy = (float)fabs(player->YPos() - tar.y);

        if (dx > dy)
            dx = dx + dy * 0.5f;
        else
            dx = dy + dx * 0.5f;

        if (dx > 40000.0f)
        {
            shortDist = FALSE;
            //MonoPrint( "Distant Effect: LONG\n" );
        }
        else
        {
            //MonoPrint( "Distant Effect: SHORT\n" );
        }
    }

    if (shooter->GetDomain() == DOMAIN_AIR)
        interval = (float)(FLIGHT_COMBAT_CHECK_INTERVAL) / (shots * 2);
    else
        interval = (float)(GROUND_COMBAT_CHECK_INTERVAL) / (shots * 2);

    // Now add visual effect by weapon type
    stype = Falcon4ClassTable[WeaponDataTable[weapon_id].Index].vuClassData.classInfo_[VU_STYPE];

    if (stype == STYPE_AAA_GUN)
    {
        if (shortDist == FALSE)
            /*
            OTWDriver.AddSfxRequest( new SfxClass( SFX_DIST_ARMOR,
             &pos,
             &vec,
             shots*2,
             interval ) );
             */
            DrawableParticleSys::PS_AddParticleEx((SFX_DIST_ARMOR + 1),
                                                  &pos,
                                                  &vec);

        // AAA - Do tracers
        /*
        OTWDriver.AddSfxRequest( new SfxClass(SFX_DIST_AIRBURSTS,
         &tar,
         shots*2,
         interval ) );
        */
        DrawableParticleSys::PS_AddParticleEx((SFX_DIST_AIRBURSTS + 1),
                                              &pos,
                                              &PSvec);
    }
    else if (stype == STYPE_ARTILLERY or stype == STYPE_MORTAR)
    {
        if (shortDist == FALSE)
            /*
            OTWDriver.AddSfxRequest( new SfxClass( SFX_DIST_ARMOR,
             &pos,
             &vec,
             shots*2,
             interval ) );
             */
            DrawableParticleSys::PS_AddParticleEx((SFX_DIST_ARMOR + 1),
                                                  &pos,
                                                  &vec);

        /*
        OTWDriver.AddSfxRequest( new SfxClass( SFX_DIST_GROUNDBURSTS,
         &tar,
         shots*2,
         interval ) );
         */
        DrawableParticleSys::PS_AddParticleEx((SFX_DIST_GROUNDBURSTS + 1),
                                              &pos,
                                              &PSvec);
    }
    else if (stype == STYPE_GUN or stype == STYPE_SMALLARMS)
    {
        if (shortDist == FALSE)
            /*
            OTWDriver.AddSfxRequest( new SfxClass( SFX_DIST_INFANTRY,
             &pos,
             &vec,
             shots*2,
             interval ) );
             */
            DrawableParticleSys::PS_AddParticleEx((SFX_DIST_INFANTRY + 1),
                                                  &pos,
                                                  &vec);

        /*
        OTWDriver.AddSfxRequest( new SfxClass( SFX_DIST_GROUNDBURSTS,
         &tar,
         shots*2,
         interval ) );
         */
        DrawableParticleSys::PS_AddParticleEx((SFX_DIST_GROUNDBURSTS + 1),
                                              &pos,
                                              &PSvec);
    }
    else if (stype == STYPE_ROCKET or stype == STYPE_MISSILE_SURF_SURF)
    {
        if (shortDist == FALSE)
            /*
            OTWDriver.AddSfxRequest( new SfxClass( SFX_DIST_ARMOR,
             &pos,
             &vec,
             shots*2,
             interval ) );
             */
            DrawableParticleSys::PS_AddParticleEx((SFX_DIST_ARMOR + 1),
                                                  &pos,
                                                  &vec);
        else
            /*
            OTWDriver.AddSfxRequest( new SfxClass( SFX_DIST_GROUNDBURSTS,
             &tar,
             shots*2,
             interval ) );
             */
            DrawableParticleSys::PS_AddParticleEx((SFX_DIST_GROUNDBURSTS + 1),
                                                  &pos,
                                                  &PSvec);
    }
    else if (stype == STYPE_MISSILE_AIR_AIR or stype == STYPE_MISSILE_AIR_GROUND or stype == STYPE_MISSILE_ANTI_SHIP)
    {
        // Add missile trail
        /*
           vec.x *= 800; // 800 fps velocity, what the heck.
           vec.y *= 800;
           vec.z *= 800;
           OTWDriver.AddSfxRequest(SFX_TRAIL_SMOKECLOUD,SFX_MOVES|SFX_USES_GRAVITY|SFX_EXPLODE_WHEN_DONE,&pos,&vec,10.0F,400.0F);
         */
        if (shortDist == FALSE)
        {
            /*
            OTWDriver.AddSfxRequest( new SfxClass( SFX_DIST_AALAUNCHES,
             &pos,
             &vec,
             shots*2,
             interval ) );
             */
            DrawableParticleSys::PS_AddParticleEx((SFX_DIST_AALAUNCHES + 1),
                                                  &pos,
                                                  &vec);
            // if target is aircraft, do countermeasures
            /*
               if (target->GetDomain() == DOMAIN_AIR)
               {
               OTWDriver.AddSfxRequest( new SfxClass( SFX_NOTRAIL_FLARE,
               &tar,
               shots*2,
               interval ) );
               }
             */
        }
        else if (target->OnGround())
            /*
            OTWDriver.AddSfxRequest( new SfxClass(SFX_DIST_GROUNDBURSTS,
             &tar,
             shots*2,
             interval ) );
             */
            DrawableParticleSys::PS_AddParticleEx((SFX_DIST_GROUNDBURSTS + 1),
                                                  &pos,
                                                  &PSvec);
        else
            /*
            OTWDriver.AddSfxRequest( new SfxClass(SFX_DIST_AIRBURSTS,
             &tar,
             shots*2,
             interval ) );
            */
            DrawableParticleSys::PS_AddParticleEx((SFX_DIST_AIRBURSTS + 1),
                                                  &pos,
                                                  &PSvec);
    }
    else if (stype == STYPE_MISSILE_SURF_AIR)
    {
        /*
        // Add missile trail
        vec.x *= 800; // 800 fps velocity, what the heck.
        vec.y *= 800;
        vec.z *= 800;
         */
        if (shortDist == FALSE)
            /*
            OTWDriver.AddSfxRequest( new SfxClass( SFX_DIST_SAMLAUNCHES,
             &pos,
             &vec,
             shots*2,
             interval ) );
             */
            DrawableParticleSys::PS_AddParticleEx((SFX_DIST_SAMLAUNCHES + 1),
                                                  &pos,
                                                  &vec);
        else
            /*
            OTWDriver.AddSfxRequest( new SfxClass(SFX_DIST_AIRBURSTS,
             &tar,
             shots*2,
             interval ) );
             */
            DrawableParticleSys::PS_AddParticleEx((SFX_DIST_AIRBURSTS + 1),
                                                  &pos,
                                                  &PSvec);
    }
    else if (stype == STYPE_BOMB or stype == STYPE_BOMB_GUIDED or stype == STYPE_BOMB_IRON or stype == STYPE_ROCKET or stype == STYPE_BOMB_GPS) //MI added GPS
    {
        /*
        OTWDriver.AddSfxRequest( new SfxClass( SFX_DIST_GROUNDBURSTS,
         &tar,
         shots*2,
         interval ) );
         */
        DrawableParticleSys::PS_AddParticleEx((SFX_DIST_GROUNDBURSTS + 1),
                                              &pos,
                                              &PSvec);
    }
}




void DoShortDistanceVisualEffects(CampEntity shooter, CampEntity target, int weapon_id, int shots)
{
    WeaponClassDataType *wc;
    Falcon4EntityClassType *classPtr;
    FalconMissileEndMessage *endMessage;
    BOOL itsaBomb = FALSE;
    int i;

    // get the weapon class table pointer
    classPtr = &Falcon4ClassTable[ weapon_id ];

    // get the weapon class pointer
    wc = (WeaponClassDataType *)Falcon4ClassTable[ weapon_id ].dataPtr;


    // what have we got for a weapon?
    if (classPtr->vuClassData.classInfo_[VU_CLASS] == CLASS_WEAPON and 
        classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_GUN)
    {
        if (wc->Flags bitand WEAP_TRACER)
        {
            // don't handle tracers
            return;
        }

        // ok, it's a shell
        // MonoPrint( "Itsa Short Distance Shell Shell\n" );

    }
    // itsa missile
    else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_MISSILE)
    {
        // MonoPrint( "Itsa Short Range Effect Missile\n" );


    }
    // itsa missile
    else if (classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_BOMB)
    {
        // MonoPrint( "Itsa Short Range Effect Bomb\n" );
        itsaBomb = TRUE;

    }
    else
    {
        // do we need to handle any other types?
        return;
    }

    // create an effect for every shot fired
    for (i = 0; i < shots; i++)
    {
        endMessage = new FalconMissileEndMessage(shooter->Id(), FalconLocalSession);
        endMessage->dataBlock.fEntityID  = shooter->Id();

        if (shooter->IsFlight())
            endMessage->dataBlock.fPilotID = (uchar)((Flight)shooter)->PickRandomPilot(target->Id().num_);
        else
            endMessage->dataBlock.fPilotID = 255;

        endMessage->dataBlock.fIndex = shooter->Type();

        endMessage->dataBlock.fCampID = shooter->GetCampID();
        endMessage->dataBlock.fSide = shooter->GetOwner();

        endMessage->dataBlock.dEntityID = target->Id();
        endMessage->dataBlock.dCampID = target->GetCampID();
        endMessage->dataBlock.dSide = target->GetOwner();

        endMessage->dataBlock.dPilotID = 0;
        endMessage->dataBlock.dCampSlot = 0;
        endMessage->dataBlock.dIndex = 0;
        endMessage->dataBlock.fWeaponUID = shooter->Id();
        endMessage->dataBlock.wIndex = (unsigned short)(weapon_id + VU_LAST_ENTITY_TYPE);

        endMessage->dataBlock.endCode = FalconMissileEndMessage::MissileKill;

        endMessage->dataBlock.xDelta = 1500.0f;
        endMessage->dataBlock.yDelta = 0.0f;
        endMessage->dataBlock.zDelta = 0.0f;

        // if target is on the ground get ground level and type
        if (target->IsBattalion() or itsaBomb)
        {
            endMessage->dataBlock.x = target->XPos() + 700.0f * PRANDFloat();
            endMessage->dataBlock.y = target->YPos() + 700.0f * PRANDFloat();
            endMessage->dataBlock.z = OTWDriver.GetGroundLevel(
                                              endMessage->dataBlock.x,
                                              endMessage->dataBlock.y);

            endMessage->dataBlock.groundType    =
                (char)OTWDriver.GetGroundType(endMessage->dataBlock.x,
                                              endMessage->dataBlock.y);
        }
        else
        {
            endMessage->dataBlock.x = target->XPos() + 700.0f * PRANDFloat();
            endMessage->dataBlock.y = target->YPos() + 700.0f * PRANDFloat();
            endMessage->dataBlock.z = target->ZPos();
            endMessage->dataBlock.groundType = -1;
        }

        FalconSendMessage(endMessage, FALSE);
    }

}


void
FireMissileAtSim(CampEntity shooter, SimBaseClass *simTarg, short weapId)
{
    MissileClass *theMissile;
    float dx, dy, dz, xydist;
    SimObjectType* tmpTargetPtr;
    SimInitDataClass initData;
    Tpoint vec, pos;
    float az, el;

    theMissile = (MissileClass *)InitAMissile(shooter, weapId, 0);


    // Need to give beam riders a pointer to the illuminating radar platform
    if (theMissile->sensorArray and theMissile->sensorArray[0]->Type() == SensorClass::RadarHoming)
    {
        // Shooter better have a radar to use this kind of weapon...
        // TODO:  Check to also ensure the radar vehicle is still alive...
        ShiAssert(shooter->GetRadarType());

        // HACK  This should never happen (hence Assert above), but since it happend tonight
        // and the data fix may not thourough by tomorrow, I'll put in this bail out case...
        if ( not shooter->GetRadarType())
        {
            // For now lets leak the missile since that should be safe.
            // Could we just delete it and be happy?
            return;
        }

        // Have the missile use the launcher's radar for guidance
        ((BeamRiderClass*)theMissile->sensorArray[0])->SetGuidancePlatform(shooter);
    }


    // get vector towards target
    dx = simTarg->XPos() - shooter->XPos();
    dy = simTarg->YPos() - shooter->YPos();
    dz = simTarg->ZPos() - shooter->ZPos();
    xydist = (float)sqrt(dx * dx + dy * dy);

    // set initial launch parameters for missile
    // NOTE:  I think MissileClass::Launch() requires this offset to be in parental object space
    //        We'll just set it to ZERO for now and let it go at that...
    theMissile->SetLaunchPosition(0.0F, 0.0F, 0.0F);
    az = (float)atan2(dy, dx);
    el = (float)atan2(-dz, xydist);

    if (shooter->OnGround())
    {
        // Don't allow ground things to fire at less than 45 degrees of initial elevation
        el = max(el, 45.0f * DTR);
    }

    theMissile->SetLaunchRotation(az, el);

    // set initial velocities
    theMissile->SetDelta(shooter->XDelta(), shooter->YDelta(), shooter->ZDelta());

    // create a target object
#ifdef DEBUG
    //tmpTargetPtr = new SimObjectType( OBJ_TAG, theMissile, simTarg );
    tmpTargetPtr = NULL;
#else
    tmpTargetPtr = new SimObjectType(simTarg);
#endif
    tmpTargetPtr->Reference();

    // Assign a shooter slot (always flight lead)
    if (shooter->IsFlight())
        theMissile->shooterPilotSlot = (uchar)((Flight)shooter)->GetFlightLeadSlot();

    // start the missile
    theMissile->Start(tmpTargetPtr);

    // Add a cloud of smoke and ground flasharound the launcher
    if (shooter->OnGround())
    {
        pos.x = shooter->XPos() ;
        pos.y = shooter->YPos() ;
        pos.z = shooter->ZPos() ;
        vec.x = 0.0f;
        vec.y = 0.0f;
        vec.z = 0.0f;
        /*
        OTWDriver.AddSfxRequest( new SfxClass( SFX_SAM_LAUNCH,
         SFX_MOVES bitor SFX_NO_GROUND_CHECK,
         &pos,
         &vec,
         2.0f,
         1.0f ) );
         */
        DrawableParticleSys::PS_AddParticleEx((SFX_SAM_LAUNCH + 1),
                                              &pos,
                                              &vec);

    }

    // We don't care about the target pointer anymore, so Release it
    tmpTargetPtr->Release();

    // put the missile into the world
    vuDatabase->/*Quick*/Insert(theMissile);

    // Setting a "Rebuild immediately" flag to ensure the missile wakes ASAP.
    // NOTE: It would be nice to put things like this into a special treatment list
    // rather than force a full bubble build each time.
    gRebuildBubbleNow = 2;

    // now we need to send a sim weapons fire message
    FalconWeaponsFire* fireMsg;

    fireMsg = new FalconWeaponsFire(shooter->Id(), FalconLocalGame);
    fireMsg->dataBlock.fEntityID = shooter->Id();
    fireMsg->dataBlock.weaponType = FalconWeaponsFire::MRM;
    fireMsg->dataBlock.fCampID = shooter->GetCampID();
    fireMsg->dataBlock.fSide = shooter->GetOwner();
    fireMsg->dataBlock.fPilotID = (uchar)theMissile->shooterPilotSlot;
    fireMsg->dataBlock.fIndex = shooter->Type();
    fireMsg->dataBlock.fWeaponID = theMissile->Type();
    fireMsg->dataBlock.dx = 0.0f;
    fireMsg->dataBlock.dy = 0.0f;
    fireMsg->dataBlock.dz = 0.0f;
    fireMsg->dataBlock.fWeaponUID = theMissile->Id();
    // fireMsg->dataBlock.fireOnOff = 1;
    fireMsg->dataBlock.targetId = simTarg->Id();

    fireMsg->RequestOutOfBandTransmit();
    fireMsg->RequestReliableTransmit();

    FalconSendMessage(fireMsg);


#if 0 // This is handled by the missile itself now...
    /*
    // Need to send a launch message if this missile is radar guided
    if (theMissile->sensorArray and 
    (theMissile->sensorArray[0]->Type() == SensorClass::Radar or
    theMissile->sensorArray[0]->Type() == SensorClass::RadarHoming)) {

    // Create and fill in the message structure
    FalconTrackMessage* trackMsg = new FalconTrackMessage( shooter->Id(), FalconLocalGame );
    ShiAssert( trackMsg );
    trackMsg->dataBlock.trackType = FalconTrackMessage::Launch;
    trackMsg->dataBlock.id = simTarg->Id();

    // Send our track list
    FalconSendMessage( trackMsg );
    }
     */
#endif
}
