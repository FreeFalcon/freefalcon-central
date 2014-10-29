#include "stdhdr.h"
#include "Graphics/Include/drawbsp.h"
#include "classtbl.h"
#include "entity.h"
#include "object.h"
#include "simweapn.h"
#include "SimDrive.h"
#include "OTWDrive.h"
#include "Campbase.h"
#include "FakeRand.h"
#include "FalcMesg.h"
#include "PlayerOp.h"
#include "MsgInc/DamageMsg.h"
#include "MsgInc/CampWeaponFireMsg.h"
#include "aircrft.h"

//sfr: added for checks
#include "InvalidBufferException.h"

//extern VuAntiDatabase *vuAntiDB;
extern int gNumWeaponsInAir;

SimWeaponClass::SimWeaponClass(FILE* filePtr) : SimMoverClass(filePtr)
{
    InitLocalData();
}

SimWeaponClass::SimWeaponClass(VU_BYTE** stream, long *rem) : SimMoverClass(stream, rem)
{
    InitLocalData();
    VU_ID vuid;
    memcpychk(&vuid, stream, sizeof(VU_ID), rem);
    VuBin<VuEntity> p(vuDatabase->Find(vuid));
    parent.reset(static_cast<FalconEntity*>(p.get()));
}

SimWeaponClass::SimWeaponClass(int type) : SimMoverClass(type)
{
    InitLocalData();
}

SimWeaponClass::~SimWeaponClass(void)
{
    CleanupLocalData();
}

void SimWeaponClass::InitData()
{
    SimMoverClass::InitData();
    InitLocalData();
}

void SimWeaponClass::InitLocalData()
{
    // every weapon inserted is sent immediately
    SetSendCreate(VuEntity::VU_SC_SEND_OOB);

    rackSlot = -1;
    nextOnRail.reset();
    parent.reset();
    shooterPilotSlot = 255;
    //parentReferenced = FALSE;
    WeaponClassDataType *wc = (WeaponClassDataType*)Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE].dataPtr;

#ifndef MISSILE_TEST_PROG
    ShiAssert(wc not_eq NULL); // JPO catch it

    if (wc) // JB 010220
    {
        if (wc->DamageType == NuclearDam)
            lethalRadiusSqrd = (float)wc->BlastRadius * 3.0f;
        else
            lethalRadiusSqrd = (float)wc->BlastRadius;
    }
    else
        lethalRadiusSqrd = 0.0f; // JB 010220

#else
    lethalRadiusSqrd = 100.0F;
#endif

    if (PlayerOptions.GetWeaponEffectiveness() == WEEnhanced)
    {
        lethalRadiusSqrd *= 1.5F;
    }

    if (PlayerOptions.GetWeaponEffectiveness() == WEExaggerated)
    {
        lethalRadiusSqrd *= 2.0F;
    }

    lethalRadiusSqrd *= lethalRadiusSqrd;
}

void SimWeaponClass::CleanupLocalData()
{
    nextOnRail.reset();
    parent.reset();
}

void SimWeaponClass::CleanupData()
{
    CleanupLocalData();
    SimMoverClass::CleanupData();
}

void SimWeaponClass::Init()
{
    SimMoverClass::Init(NULL);
}

int SimWeaponClass::Sleep()
{
    if (SimDriver.RunningInstantAction())
    {
        if (parent.get() == SimDriver.GetPlayerEntity())
        {
            gNumWeaponsInAir --;
        }
    }

    parent.reset();
    /*if (parentReferenced)
    {
       parentReferenced = FALSE;
       VuDeReferenceEntity (parent);
       parent = NULL;
    }*/

    return SimMoverClass::Sleep();
}

int SimWeaponClass::Wake(void)
{
    if (SimDriver.RunningInstantAction())
    {
        if (parent.get() == SimDriver.GetPlayerEntity())
        {
            ++gNumWeaponsInAir;
        }
    }

    return SimMoverClass::Wake();
    //   vuAntiDB->Remove (this);
}

int SimWeaponClass::GetRadarType(void)
{
    WeaponClassDataType *wc = (WeaponClassDataType*)Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE].dataPtr;

    ShiAssert(wc);

    if (wc)
        return wc->RadarType;
    else
        return 0;
}

int SimWeaponClass::Exec(void)
{
    return SimMoverClass::Exec();
}

// function interface
// serialization functions
int SimWeaponClass::SaveSize()
{
    return SimMoverClass::SaveSize() + sizeof(VU_ID);
}

// Try this code for missile removal for remote entities
#if 0
int SimWeaponClass::SaveSize()
{
    int retval = 0;

    // Streamify the data
    retval += sizeof(VU_ID);
    retval += sizeof(VU_ID);
    retval += sizeof(char);
    retval += sizeof(char);

    return retval;
}

int SimWeaponClass::Save(VU_BYTE **stream)
{
    int retval = 0;

    ShiAssert(parent);

    if ( not parent)
        return NULL;

    VU_ID parentId = parent->Id();
    VU_ID myId = parent->Id();
    char hardPoint;
    char slot = GetRackSlot();

    // Streamify the data
    memcpy(*stream, &parentId, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    retval += sizeof(VU_ID);

    memcpy(*stream, &myId, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    retval += sizeof(VU_ID);

    memcpy(*stream, &hardPoint, sizeof(char));
    *stream += sizeof(char);
    retval += sizeof(char);

    memcpy(*stream, &hardPoint, sizeof(char));
    *stream += sizeof(char);
    retval += sizeof(char);

    return retval;
}
#endif

int SimWeaponClass::Save(VU_BYTE **stream)
{
    int retval;

    ShiAssert(parent);

    if ( not parent)
        return NULL;

    VU_ID vuid = parent->Id();

    retval = SimMoverClass::Save(stream);

    memcpy(*stream, &vuid, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    retval += sizeof(VU_ID);

    return retval;
}

int SimWeaponClass::Save(FILE *file)
{
    int retval;

    retval = SimMoverClass::Save(file);

    return (retval);
}

int SimWeaponClass::Handle(VuFullUpdateEvent *event)
{
    return (SimMoverClass::Handle(event));
}


int SimWeaponClass::Handle(VuPositionUpdateEvent *event)
{
    return (SimMoverClass::Handle(event));
}

int SimWeaponClass::Handle(VuTransferEvent *event)
{
    return (SimMoverClass::Handle(event));
}

void SimWeaponClass::SetDead(int flag)
{
    if (flag/* and parentReferenced*/)
    {
        /*parentReferenced = FALSE;
        VuDeReferenceEntity (parent);*/
        parent.reset();
    }

    SimMoverClass::SetDead(flag);
}

void SimWeaponClass::SendDamageMessage(FalconEntity *testObject, float rangeSquare, int damageType)
{
#ifndef MISSILE_TEST_PROG

    FalconDamageMessage* message;
    float normBlastDist;
    float lethalCone;

    // Don't damage other weapons
    if (testObject->IsMissile() or testObject->IsBomb())
        return;

    // Adjust damage for distance:
    WeaponClassDataType* wc = (WeaponClassDataType *)Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE].dataPtr;
    ShiAssert(wc);
    // edg: calculate a normalized blast Dist
    normBlastDist = (lethalRadiusSqrd - rangeSquare) / (lethalRadiusSqrd);

    // quadratic dropoff
    normBlastDist *= normBlastDist;

    // Player setting damage modifier
    if (PlayerOptions.GetWeaponEffectiveness() == WEEnhanced)
        lethalCone = 0.7f;
    else if (PlayerOptions.GetWeaponEffectiveness() == WEExaggerated)
        lethalCone = 0.8f;
    else
        lethalCone = 0.9f;

    // pre gilman: within an 70% dist from blast there's a 100% chance
    // of being hit.  Outside that there's a chance of nooot being hit
    // at all based on the normalized blast distance

    //TJ_changes since before all missile were of rangeSquare = 0 this function never returned here for
    //air to air missiles ... however with  new calculation of damage we should always let the damage happen
    //or modify this to be less sensitive
    /*
    if (normBlastDist < lethalCone and PRANDFloatPos() * 2.0f > normBlastDist)
    {
     return;
    }
    */ //end TJ_changes

    message = new FalconDamageMessage(testObject->Id(), FalconLocalGame);
    message->dataBlock.fEntityID  = parent->Id();
    message->dataBlock.fCampID = parent->GetCampID();
    message->dataBlock.fSide   = static_cast<uchar>(parent->GetCountry());

    if (parent->IsSimObjective() or parent->IsCampaign())
        message->dataBlock.fPilotID   = 255;
    else
        message->dataBlock.fPilotID   = shooterPilotSlot;

    message->dataBlock.fIndex     = parent->Type();
    message->dataBlock.fWeaponID  = Type();
    message->dataBlock.fWeaponUID = Id();

    message->dataBlock.dEntityID  = testObject->Id();
    message->dataBlock.dCampID = testObject->GetCampID();
    message->dataBlock.dSide   = static_cast<uchar>(testObject->GetCountry());

    if (testObject->IsSim() and testObject->IsMover())
        message->dataBlock.dPilotID   = ((SimMoverClass*)testObject)->pilotSlot;
    else
        message->dataBlock.dPilotID   = 255;

    message->dataBlock.dIndex     = testObject->Type();

    //MI special case nukes
    if (wc and wc->DamageType == NuclearDam)
    {
        message->dataBlock.damageStrength = normBlastDist * (wc->Strength * 2000000.0F);
        message->dataBlock.damageRandomFact = 5.0F; // nukes have exaggerated Damage factor
    }
    else
    {
        message->dataBlock.damageRandomFact = PRANDFloat();
        message->dataBlock.damageStrength = normBlastDist * wc->Strength;
    }


    if (normBlastDist >= lethalCone)
    {
        // Direct hit
        // message->dataBlock.damageRandomFact += 1.0F;
        message->dataBlock.damageType = damageType;
    }
    else
    {
        message->dataBlock.damageType = FalconDamageType::ProximityDamage;
    }

    // Player setting damage modifier
    if (PlayerOptions.GetWeaponEffectiveness() == WEEnhanced)
        message->dataBlock.damageRandomFact += 2.0F;

    if (PlayerOptions.GetWeaponEffectiveness() == WEExaggerated)
        message->dataBlock.damageRandomFact += 4.0F;

    message->RequestOutOfBandTransmit();
    FalconSendMessage(message, TRUE);
#endif
}


short SimWeaponClass::GetWeaponId()
{
    Falcon4EntityClassType* classPtr;
    WeaponClassDataType* wc;
    classPtr = (Falcon4EntityClassType*)EntityType();
    wc = (WeaponClassDataType*)classPtr->dataPtr;
    return (short)
           (((int)Falcon4ClassTable[wc->Index].dataPtr - (int)WeaponDataTable) / sizeof(WeaponClassDataType))
           ;
}

SimWeaponDataType      *SimWeaponClass::GetSWD(void)
{
    Falcon4EntityClassType* classPtr;

    classPtr = (Falcon4EntityClassType*)EntityType();
    return  &SimWeaponDataTable[classPtr->vehicleDataIndex];
}

Falcon4EntityClassType *SimWeaponClass::GetCT(void)
{
    return (Falcon4EntityClassType*)EntityType();
}

WeaponClassDataType    *SimWeaponClass::GetWCD(void)
{
    Falcon4EntityClassType* classPtr;

    classPtr = (Falcon4EntityClassType*)EntityType();
    return (WeaponClassDataType*)classPtr->dataPtr;
}

