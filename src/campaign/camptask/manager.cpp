#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "Listadt.h"
#include "CampBase.h"
#include "Team.h"
#include "Weather.h"
#include "Manager.h"
#include "MsgInc/CampTaskingMsg.h"
#include "classtbl.h"
#include "falcsess.h"
#include "campaign.h"

//sfr: included for buffer checks
#include "InvalidBufferException.h"


#ifdef CAMPTOOL
// Renaming tool stuff
extern VU_ID_NUMBER RenameTable[65536];
extern int gRenameIds;
#endif

// ====================================
// Manager Class
// ====================================

CampManagerClass::CampManagerClass(ushort type, Team t)
    : FalconEntity(type, GetIdFromNamespace(VolatileNS))
{
    // these are read from files by each side
    SetSendCreate(VuEntity::VU_SC_DONT_SEND);
    InitLocalData(t);
}

CampManagerClass::CampManagerClass(VU_BYTE **stream, long *rem)
    : FalconEntity(VU_LAST_ENTITY_TYPE, GetIdFromNamespace(VolatileNS))
{
    // these are read from files by each side
    SetSendCreate(VuEntity::VU_SC_DONT_SEND);

    // Read vu stuff here
    memcpychk(&share_.id_, stream, sizeof(VU_ID), rem);
    memcpychk(&share_.ownerId_, stream, sizeof(VU_ID), rem);
    memcpychk(&share_.entityType_, stream, sizeof(ushort), rem);
    SetEntityType(share_.entityType_);

    memcpychk(&managerFlags, stream, sizeof(short), rem);
    memcpychk(&owner, stream, sizeof(Team), rem);

    if (FalconLocalGame)
    {
        SetAssociation(FalconLocalGame->Id());
    }
}

CampManagerClass::CampManagerClass(FILE *file)
    : FalconEntity(VU_LAST_ENTITY_TYPE, GetIdFromNamespace(VolatileNS))
{
    // these are read from files by each side
    SetSendCreate(VuEntity::VU_SC_DONT_SEND);

    // Read vu stuff here
    fread(&share_.id_, sizeof(VU_ID), 1, file);
    fread(&share_.entityType_, sizeof(ushort), 1, file);
    SetEntityType(share_.entityType_);

#ifdef DEBUG
    // VU_ID_NUMBERs moved to 32 bits
    share_.id_.num_ and_eq 0xffff;
#endif
    //#ifdef CAMPTOOL
    // if (gRenameIds)
    // {
    // VU_ID new_id = FalconNullId;
    //
    // // Rename this ID
    // for (new_id.num_ = FIRST_NON_VOLATILE_VU_ID_NUMBER; new_id.num_ < LAST_NON_VOLATILE_VU_ID_NUMBER; new_id.num_++)
    // {
    // if ( not vuDatabase->Find(new_id))
    // {
    // RenameTable[share_.id_.num_] = new_id.num_;
    // share_.id_ = new_id;
    // break;
    // }
    // }
    // }
    //#endif

    // Set the owner to the game master.
    if ((FalconLocalGame) and ( not FalconLocalGame->IsLocal()))
        SetOwnerId(FalconLocalGame->OwnerId());

    fread(&managerFlags, sizeof(short), 1, file);
    fread(&owner, sizeof(Team), 1, file);

    // Associate this entity with the game owner.
    if (FalconLocalGame)
        SetAssociation(FalconLocalGame->Id());
}

CampManagerClass::~CampManagerClass(void)
{
    // KCK HACK: Try to get rid of any managers which leaked through
    int t;

    for (t = 0; t < NUM_TEAMS; t++)
    {
        if (TeamInfo[t])
        {
            if (TeamInfo[t]->atm == this)
            {
                ShiAssert( not "Manager reference problem");
                TeamInfo[t]->atm = NULL;
            }

            if (TeamInfo[t]->gtm == this)
            {
                ShiAssert( not "Manager reference problem");
                TeamInfo[t]->gtm = NULL;
            }

            if (TeamInfo[t]->ntm == this)
            {
                ShiAssert( not "Manager reference problem");
                TeamInfo[t]->ntm = NULL;
            }
        }
    }
}

void CampManagerClass::InitData()
{
    FalconEntity::InitData();
    InitLocalData(owner);
}

void CampManagerClass::InitLocalData(Team t)
{
    managerFlags = 0;
    owner = t;

    if (FalconLocalGame)
    {
        SetAssociation(FalconLocalGame->Id());
    }
}

int CampManagerClass::SaveSize(void)
{
    return sizeof(VU_ID)
           + sizeof(VU_ID)
           + sizeof(ushort)
           + sizeof(short)
           + sizeof(Team);
}

int CampManagerClass::Save(VU_BYTE **stream)
{
    // Write vu stuff here
    memcpy(*stream, &share_.id_, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    memcpy(*stream, &share_.ownerId_, sizeof(VU_ID));
    *stream += sizeof(VU_ID);
    memcpy(*stream, &share_.entityType_, sizeof(ushort));
    *stream += sizeof(ushort);

    memcpy(*stream, &managerFlags, sizeof(short));
    *stream += sizeof(short);
    memcpy(*stream, &owner, sizeof(Team));
    *stream += sizeof(Team);
    return CampManagerClass::SaveSize();
}

int CampManagerClass::Save(FILE *file)
{
    int retval = 0;

    if ( not file)
        return 0;

    // Write vu stuff here
    retval += fwrite(&share_.id_, sizeof(VU_ID), 1, file);
    retval += fwrite(&share_.entityType_, sizeof(ushort), 1, file);

    retval += fwrite(&managerFlags, sizeof(short), 1, file);
    retval += fwrite(&owner, sizeof(Team), 1, file);
    return retval;
}

void CampManagerClass::SendMessage(VU_ID from, short msg, short d1, short d2, short d3)
{
    VuTargetEntity *target = (VuTargetEntity*) vuDatabase->Find(OwnerId());
    FalconCampTaskingMessage *message = new FalconCampTaskingMessage(Id(), target);

    if (managerFlags bitand CTM_MUST_BE_OWNED and not IsLocal())
        return;

    message->dataBlock.from = from;
    message->dataBlock.team = owner;
    message->dataBlock.messageType = msg;
    message->dataBlock.data1 = d1;
    message->dataBlock.data2 = d2;
    message->dataBlock.data3 = (void*)d3;
    FalconSendMessage(message, TRUE);
}

// event handlers
int CampManagerClass::Handle(VuEvent *event)
{
    //Event Handler
    return (VuEntity::Handle(event));
}

int CampManagerClass::Handle(VuFullUpdateEvent *event)
{
    return (VuEntity::Handle(event));
}

int CampManagerClass::Handle(VuPositionUpdateEvent *event)
{
    return (VuEntity::Handle(event));
}

int CampManagerClass::Handle(VuEntityCollisionEvent *event)
{
    return (VuEntity::Handle(event));
}

int CampManagerClass::Handle(VuTransferEvent *event)
{
    return (VuEntity::Handle(event));
}

int CampManagerClass::Handle(VuSessionEvent *event)
{
    return (VuEntity::Handle(event));
}

VU_ERRCODE CampManagerClass::InsertionCallback(void)
{
    // sfr: why do we need this ref/unref??
    //VuReferenceEntity(this);
    ShiAssert(TeamInfo[owner]);

    if (TeamInfo[owner])
    {
        if (EntityType()->classInfo_[VU_DOMAIN] == DOMAIN_AIR)
        {
            ShiAssert( not TeamInfo[owner]->atm);

            if (TeamInfo[owner]->atm)
                VuDeReferenceEntity(TeamInfo[owner]->atm);

            TeamInfo[owner]->atm = (AirTaskingManagerClass*) this;
        }
        else if (EntityType()->classInfo_[VU_DOMAIN] == DOMAIN_LAND)
        {
            ShiAssert( not TeamInfo[owner]->gtm);

            if (TeamInfo[owner]->gtm)
                VuDeReferenceEntity(TeamInfo[owner]->gtm);

            TeamInfo[owner]->gtm = (GroundTaskingManagerClass*) this;
        }
        else if (EntityType()->classInfo_[VU_DOMAIN] == DOMAIN_SEA)
        {
            ShiAssert( not TeamInfo[owner]->ntm);

            if (TeamInfo[owner]->ntm)
                VuDeReferenceEntity(TeamInfo[owner]->ntm);

            TeamInfo[owner]->ntm = (NavalTaskingManagerClass*) this;
        }
    }

    return FalconEntity::InsertionCallback();
}

VU_ERRCODE CampManagerClass::RemovalCallback(void)
{
    // ShiAssert(TeamInfo[owner]);
    if (TeamInfo[owner])
    {
        if (EntityType()->classInfo_[VU_DOMAIN] == DOMAIN_AIR)
            TeamInfo[owner]->atm = NULL;
        else if (EntityType()->classInfo_[VU_DOMAIN] == DOMAIN_LAND)
            TeamInfo[owner]->gtm = NULL;
        else if (EntityType()->classInfo_[VU_DOMAIN] == DOMAIN_SEA)
            TeamInfo[owner]->ntm = NULL;
    }

    // sfr: why do we need to ref/unref ourselves?
    //VuDeReferenceEntity(this);
    return FalconEntity::RemovalCallback();
}

// ===============================
// Global functions
// ===============================

//Sfr: chg here too
VuEntity* NewManager(short type, VU_BYTE **stream, long *rem)
{
    VuEntity *retval = 0;
    VuEntityType* classPtr = VuxType(type);

    CampEnterCriticalSection();

    switch (classPtr->classInfo_[VU_DOMAIN])
    {
        case (DOMAIN_AIR):
        {
            retval = (VuEntity*) new AirTaskingManagerClass(stream, rem);
            break;
        }

        case (DOMAIN_LAND):
        {
            retval = (VuEntity*) new GroundTaskingManagerClass(stream, rem);
            break;
        }

        case (DOMAIN_SEA):
        {
            retval = (VuEntity*) new NavalTaskingManagerClass(stream, rem);
            break;
        }

        case (DOMAIN_ABSTRACT):
        {
            retval = (VuEntity*) new TeamClass(stream, rem);
            break;
        }

        default:
        {
            char err[50];
            sprintf(err, "%s %d: invalid domain", __FILE__, __LINE__);
            throw InvalidBufferException(err);
        }
    }

    CampLeaveCriticalSection();

    return retval;
}
