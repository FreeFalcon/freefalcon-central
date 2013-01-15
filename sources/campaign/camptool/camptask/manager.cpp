#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "Listadt.h"
#include "CampBase.h"
#include "Team.h"
#include "weather.h"
#include "Manager.h"
#include "MsgInc\CampTaskingMsg.h"
#include "classtbl.h"
#include "falcsess.h"
#include "campaign.h"

#ifdef CAMPTOOL
// Renaming tool stuff
extern VU_ID_NUMBER RenameTable[65536];
extern int gRenameIds;
#endif

// ====================================
// Manager Class
// ====================================

CampManagerClass::CampManagerClass (ushort type, Team t) : FalconEntity (type)
	{
	managerFlags = 0;
	owner = t;	
	if (FalconLocalGame)
		SetAssociation(FalconLocalGame->Id());
	}

CampManagerClass::CampManagerClass (VU_BYTE **stream) : FalconEntity (VU_LAST_ENTITY_TYPE)
	{
	// Read vu stuff here
	memcpy(&share_.id_, *stream, sizeof(VU_ID));				*stream += sizeof(VU_ID);
	memcpy(&share_.ownerId_, *stream, sizeof(VU_ID));			*stream += sizeof(VU_ID);
	memcpy(&share_.entityType_, *stream, sizeof(ushort));		*stream += sizeof(ushort);
	SetEntityType(share_.entityType_);

	memcpy(&managerFlags, *stream, sizeof(short));				*stream += sizeof(short);
	memcpy(&owner, *stream, sizeof(Team));						*stream += sizeof(Team);
	if (FalconLocalGame)
		SetAssociation(FalconLocalGame->Id());
	}

CampManagerClass::CampManagerClass (FILE *file) : FalconEntity (VU_LAST_ENTITY_TYPE)
	{
	// Read vu stuff here
	fread(&share_.id_, sizeof(VU_ID), 1, file);
	fread(&share_.entityType_, sizeof(ushort), 1, file);
	SetEntityType(share_.entityType_);

#ifdef DEBUG
	// VU_ID_NUMBERs moved to 32 bits
	share_.id_.num_ &= 0xffff;
#endif
#ifdef CAMPTOOL
	if (gRenameIds)
		{
		VU_ID		new_id = FalconNullId;

		// Rename this ID
		for (new_id.num_ = FIRST_NON_VOLITILE_VU_ID_NUMBER; new_id.num_ < LAST_NON_VOLITILE_VU_ID_NUMBER; new_id.num_++)
			{
			if (!vuDatabase->Find(new_id))
				{
				RenameTable[share_.id_.num_] = new_id.num_;
				share_.id_ = new_id;
				break;
				}
			}
		}
#endif

	// Set the owner to the game master.
	if ((FalconLocalGame) && (!FalconLocalGame->IsLocal()))
		SetOwnerId(FalconLocalGame->OwnerId());

	fread(&managerFlags, sizeof(short), 1, file);				
	fread(&owner, sizeof(Team), 1, file);			

	// Associate this entity with the game owner.
	if (FalconLocalGame)
		SetAssociation(FalconLocalGame->Id());
	}

CampManagerClass::~CampManagerClass (void)
	{
	// KCK HACK: Try to get rid of any managers which leaked through
	int			t;
	for (t=0; t<NUM_TEAMS; t++)
		{
		if (TeamInfo[t])
			{
			if (TeamInfo[t]->atm == this)
				{
				ShiAssert ( !"Manager reference problem" );
				TeamInfo[t]->atm = NULL;
				}
			if (TeamInfo[t]->gtm == this)
				{
				ShiAssert ( !"Manager reference problem" );
				TeamInfo[t]->gtm = NULL;
				}
			if (TeamInfo[t]->ntm == this)
				{
				ShiAssert ( !"Manager reference problem" );
				TeamInfo[t]->ntm = NULL;
				}
			}
		}
	}

int CampManagerClass::SaveSize (void)
	{
	return sizeof(VU_ID)
		+ sizeof(VU_ID)
		+ sizeof(ushort)
		+ sizeof(short)
		+ sizeof(Team);
	}

int CampManagerClass::Save (VU_BYTE **stream)
	{
	// Write vu stuff here
	memcpy(*stream, &share_.id_, sizeof(VU_ID));				*stream += sizeof(VU_ID);
	memcpy(*stream, &share_.ownerId_, sizeof(VU_ID));			*stream += sizeof(VU_ID);
	memcpy(*stream, &share_.entityType_, sizeof(ushort));		*stream += sizeof(ushort);

	memcpy(*stream, &managerFlags, sizeof(short));				*stream += sizeof(short);
	memcpy(*stream, &owner, sizeof(Team));						*stream += sizeof(Team);
	return CampManagerClass::SaveSize();
	}

int CampManagerClass::Save (FILE *file)
	{
	int	retval = 0;

	if (!file)
		return 0;
	// Write vu stuff here
	retval += fwrite(&share_.id_, sizeof(VU_ID), 1, file);
	retval += fwrite(&share_.entityType_, sizeof(ushort), 1, file);

	retval += fwrite(&managerFlags, sizeof(short), 1, file);				
	retval += fwrite(&owner, sizeof(Team), 1, file);			
	return retval;
	}

void CampManagerClass::SendMessage (VU_ID from, short msg, short d1, short d2, short d3)
	{
	VuTargetEntity				*target = (VuTargetEntity*) vuDatabase->Find(OwnerId());
	FalconCampTaskingMessage	*message = new FalconCampTaskingMessage(Id(), target);

	if (managerFlags & CTM_MUST_BE_OWNED && !IsLocal())
		return;
	message->dataBlock.from = from;
	message->dataBlock.team = owner;
	message->dataBlock.messageType = msg;
	message->dataBlock.data1 = d1;
	message->dataBlock.data2 = d2;
	message->dataBlock.data3 = (void*)d3;
	FalconSendMessage(message,TRUE);
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
	VuReferenceEntity(this);
	ShiAssert(TeamInfo[owner]);
	if (TeamInfo[owner])
		{
		if (EntityType()->classInfo_[VU_DOMAIN] == DOMAIN_AIR)
			{
			ShiAssert (!TeamInfo[owner]->atm);
			if (TeamInfo[owner]->atm)
				VuDeReferenceEntity(TeamInfo[owner]->atm);
			TeamInfo[owner]->atm = (AirTaskingManagerClass*) this;
			}
		else if (EntityType()->classInfo_[VU_DOMAIN] == DOMAIN_LAND)
			{
			ShiAssert (!TeamInfo[owner]->gtm);
			if (TeamInfo[owner]->gtm)
				VuDeReferenceEntity(TeamInfo[owner]->gtm);
			TeamInfo[owner]->gtm = (GroundTaskingManagerClass*) this;
			}
		else if (EntityType()->classInfo_[VU_DOMAIN] == DOMAIN_SEA)
			{
			ShiAssert (!TeamInfo[owner]->ntm);
			if (TeamInfo[owner]->ntm)
				VuDeReferenceEntity(TeamInfo[owner]->ntm);
			TeamInfo[owner]->ntm = (NavalTaskingManagerClass*) this;
			}
		}
	return FalconEntity::InsertionCallback();
	}

VU_ERRCODE CampManagerClass::RemovalCallback(void)
	{
//	ShiAssert(TeamInfo[owner]);
	if (TeamInfo[owner])
		{
		if (EntityType()->classInfo_[VU_DOMAIN] == DOMAIN_AIR)
			TeamInfo[owner]->atm = NULL;
		else if (EntityType()->classInfo_[VU_DOMAIN] == DOMAIN_LAND)
			TeamInfo[owner]->gtm = NULL;
		else if (EntityType()->classInfo_[VU_DOMAIN] == DOMAIN_SEA)
			TeamInfo[owner]->ntm = NULL;
		}
	VuDeReferenceEntity(this);
	return FalconEntity::RemovalCallback();
	}

// ===============================
// Global functions
// ===============================

VuEntity* NewManager (short type, VU_BYTE *stream)
	{
	VuEntity *retval = 0;
	VuEntityType* classPtr = VuxType(type);

//#ifndef NDEBUG
//	MonoPrint ("Got manager type %d.\n",classPtr->classInfo_[VU_DOMAIN]);
//#endif

	CampEnterCriticalSection();
	if (classPtr->classInfo_[VU_DOMAIN] == DOMAIN_AIR)
		retval = (VuEntity*) new AirTaskingManagerClass (&stream);
	else if (classPtr->classInfo_[VU_DOMAIN] == DOMAIN_LAND)
		retval = (VuEntity*) new GroundTaskingManagerClass (&stream);
	else if (classPtr->classInfo_[VU_DOMAIN] == DOMAIN_SEA)
		retval = (VuEntity*) new NavalTaskingManagerClass (&stream);
	else if (classPtr->classInfo_[VU_DOMAIN] == DOMAIN_ABSTRACT)
		retval = (VuEntity*) new TeamClass (&stream);
	CampLeaveCriticalSection();

	return retval;
	}
