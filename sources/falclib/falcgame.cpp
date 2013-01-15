// FalcGame.cpp
// Falcon's game subclass
//====================================================================================

#include <wtypes.h>
#include "f4vu.h"
#include "vusessn.h"
#include "tchar.h"
#include "FalcSess.h"
#include "ui/include/uicomms.h"
#include "classtbl.h"
#include "CampBase.h"
#include "F4Thread.h"
#include "CmpClass.h"
#include "sim/include/SimBase.h"
#include "rules.h"
#include "dispcfg.h"
#include "ui95/chandler.h"
#include "FalcGame.h"
#include "InvalidBufferException.h"
//#include "datadir.h"

extern C_Handler *gMainHandler;
extern int F4GameType;

void INFOSetupRulesControls(void);

static char PasswordKey1[]="Coming soon to Stores Everywhere... Falcon 4.0!!";
static char PasswordKey2[]="This is another stupid advertisement... hehehe!!";

// constructors & destructor
FalconGameEntity::FalconGameEntity(ulong domainMask, char *gameName) : VuGameEntity(domainMask, gameName)
{
	gameType = game_PlayerPool;
	// KCK: To keep this from conflicting with entities we plan to load, force
	// the creater to something non-zero for single player games.
	if (!share_.id_.creator_){
		share_.id_.creator_ = 1;
		// Make a new collection with a filter to match
		VuSessionFilter filter(Id());
		sessionCollection_->Unregister();
		delete sessionCollection_;
		sessionCollection_ = new VuOrderedList(&filter);
		sessionCollection_->Register();
	}
	SetEntityType((unsigned short)(F4GameType+VU_LAST_ENTITY_TYPE));
}

FalconGameEntity::FalconGameEntity(VU_BYTE** stream, long *rem) : VuGameEntity (0,"VuGame")
{
	VU_ID			sessionid(0, 0);
	VuSessionEntity *session;
	uchar			size;

	// VuEntity part
	memcpychk(&share_.entityType_, stream, sizeof(share_.entityType_), rem);
	memcpychk(&share_.flags_, stream, sizeof(share_.flags_), rem);
	memcpychk(&share_.id_.creator_, stream, sizeof(share_.id_.creator_), rem);
	memcpychk(&share_.id_.num_, stream, sizeof(share_.id_.num_), rem);

	SetEntityType(share_.entityType_);

	VuSessionFilter filter(Id());
	sessionCollection_ = new VuOrderedList(&filter);
	sessionCollection_->Register();

	// VuTarget part
	memcpychk(&share_.ownerId_.creator_, stream, sizeof(share_.ownerId_.creator_), rem);
	memcpychk(&share_.ownerId_.num_, stream, sizeof(share_.ownerId_.num_), rem);
	memcpychk(&share_.assoc_.creator_, stream, sizeof(share_.assoc_.creator_), rem);
	memcpychk(&share_.assoc_.num_, stream, sizeof(share_.assoc_.num_), rem);
	memset(&bestEffortComms_, 0, sizeof(VuCommsContext));
	bestEffortComms_.status_ = VU_CONN_INACTIVE;
	memset(&reliableComms_, 0, sizeof(VuCommsContext));
	reliableComms_.status_ = VU_CONN_INACTIVE;
	reliableComms_.reliable_ = TRUE;

	// vuGroupEntity part
	memcpychk(&sessionMax_, stream, sizeof(ushort), rem);
	short count = 0;
	memcpychk(&count, stream, sizeof(short), rem);
	for (int i = 0; i < count; i++) 
	{
		memcpychk(&sessionid, stream, sizeof(VU_ID), rem);
		if (sessionid == vuLocalSession)
			selfIndex_ = i;
		session = (VuSessionEntity *)vuDatabase->Find(sessionid);
		if (session)
		{
			sessionCollection_->Insert(session);
		}
		//	    AddSession(sessionid);
	}

	memcpychk (&domainMask_, stream, sizeof(ulong), rem);
	memcpychk (&gameType, stream, sizeof (FalconGameType), rem);
	memcpychk (&rules,	stream, sizeof (class RulesClass), rem);
	memcpychk (&size, stream, sizeof (uchar), rem);
	gameName_ = new char[size+1];
	memcpychk (gameName_, stream, sizeof(char)*size, rem);
	gameName_[size]=0;
}

FalconGameEntity::FalconGameEntity(FILE* filePtr) : VuGameEntity (filePtr)
	{
	fread (&gameType, sizeof (FalconGameType), 1, filePtr);
	fread (&rules, sizeof (class RulesClass), 1, filePtr);
	}

FalconGameEntity::~FalconGameEntity(void)
	{
	}

int FalconGameEntity::Save(VU_BYTE** stream)
	{
	int					count = sessionCollection_->Count();
	VuSessionsIterator	iter(this);
	VuSessionEntity		*ent;
	VU_ID				id;
	int					start = (int) *stream;
	uchar				size;

	// VuEntity part
	memcpy(*stream, &share_.entityType_, sizeof(share_.entityType_));
        *stream += sizeof(share_.entityType_);
	memcpy(*stream, &share_.flags_, sizeof(share_.flags_));
        *stream += sizeof(share_.flags_);
	memcpy(*stream, &share_.id_.creator_, sizeof(share_.id_.creator_));
        *stream += sizeof(share_.id_.creator_);
	memcpy(*stream, &share_.id_.num_, sizeof(share_.id_.num_));
        *stream += sizeof(share_.id_.num_);

	// VuTarget part
	memcpy(*stream, &share_.ownerId_.creator_, sizeof(share_.ownerId_.creator_));
		*stream += sizeof(share_.ownerId_.creator_);
	memcpy(*stream, &share_.ownerId_.num_, sizeof(share_.ownerId_.num_));
        *stream += sizeof(share_.ownerId_.num_);
	memcpy(*stream, &share_.assoc_.creator_, sizeof(share_.assoc_.creator_));
        *stream += sizeof(share_.assoc_.creator_);
	memcpy(*stream, &share_.assoc_.num_, sizeof(share_.assoc_.num_));
        *stream += sizeof(share_.assoc_.num_);

	// vuGroupEntity part
	memcpy(*stream, &sessionMax_, sizeof(ushort));			*stream += sizeof(ushort);
	memcpy(*stream, &count, sizeof(short));					*stream += sizeof(short);
	ent = iter.GetFirst();
	while (ent) 
		{
		id = ent->Id();
		memcpy(*stream, &id, sizeof(VU_ID));				*stream += sizeof(VU_ID);
		ent = iter.GetNext();
		}

	memcpy(*stream, &domainMask_, sizeof(ulong));			*stream += sizeof(ulong);
	memcpy(*stream, &gameType, sizeof (FalconGameType));	*stream += sizeof (FalconGameType);
	memcpy(*stream, &rules,	sizeof (class RulesClass));		*stream += sizeof (class RulesClass);

	size = (uchar) strlen(gameName_);
	memcpy (*stream, &size, sizeof (uchar));				*stream += sizeof (uchar);
	memcpy (*stream, gameName_, sizeof(char)*size);		*stream += sizeof (char)*size;

	return (int)*stream - start;
	}

int FalconGameEntity::Save(FILE* filePtr)
	{
	VuGameEntity::Save(filePtr);
	fwrite (&gameType, sizeof (FalconGameType), 1, filePtr);
	fwrite (&rules, sizeof (class RulesClass), 1, filePtr);
	return SaveSize();
	}

int FalconGameEntity::SaveSize (void)
	{
	int count = sessionCollection_->Count();
	int saveSize =	sizeof(share_.entityType_) +
					sizeof(share_.flags_) +
					sizeof(share_.id_.creator_) +
					sizeof(share_.id_.num_) +
					sizeof(share_.ownerId_.creator_) +
					sizeof(share_.ownerId_.num_) +
					sizeof(share_.assoc_.creator_) +
					sizeof(share_.assoc_.num_) +
					sizeof(ushort) +
					sizeof(short) +
					sizeof(VU_ID) * count + 
					sizeof(ulong) +
					LocalSize();
	return saveSize;
	}

int FalconGameEntity::LocalSize() const {
	uchar size = (uchar) strlen(gameName_);
	return sizeof (uchar) + size + sizeof(FalconGameType) + sizeof(class RulesClass);
}

void FalconGameEntity::SetGameType (FalconGameType type)
{
	gameType = type; 
	DoFullUpdate();
}

void FalconGameEntity::EncipherPassword (char *data,long size)
{
	int i;
	for(i=0;i<size;i++)
		data[i] = (char)((data[i]^PasswordKey1[i])^PasswordKey2[i]);
}

long FalconGameEntity::CheckPassword (_TCHAR *passwd)
{
	_TCHAR buffer[RUL_PW_LEN];
	memset(buffer,0,sizeof(_TCHAR)*RUL_PW_LEN);
	_tcscpy(buffer,passwd);
	EncipherPassword((char*)buffer,sizeof(_TCHAR)*RUL_PW_LEN);

	if(memcmp(buffer,rules.Password,sizeof(_TCHAR)*RUL_PW_LEN))
		return(FALSE);
	return(TRUE);
}

void FalconGameEntity::UpdateRules (RulesStruct *newrules)
{
	rules.LoadRules(newrules);
	EncipherPassword(rules.Password,sizeof(_TCHAR)*RUL_PW_LEN);
	if (VuState() == VU_MEM_ACTIVE){
		DoFullUpdate();
	}
}

void FalconGameEntity::DoFullUpdate (void)
{
	VuEvent *event = new VuFullUpdateEvent(this, vuGlobalGroup);
	VuMessageQueue::PostVuMessage(event);
}

FalconGameType FalconGameEntity::GetGameType (void)
{
	// KCK Hack to avoid having to type "if (FalconLocalGame && FalconLocalGame->GetGameType ...)"
	if (!this) 
		return game_PlayerPool; 
	return gameType; 
}

VU_ERRCODE FalconGameEntity::Handle(VuFullUpdateEvent *event)
{
	FalconGameEntity	*tmpGame = (FalconGameEntity*)(event->expandedData_.get());
	VuSessionsIterator	iter(tmpGame);
	VuSessionEntity		*ent;

	if (IsLocal())
		return 0;
	
	int dirty = FALSE;
	if(FalconLocalSession->GetFlyState() == FLYSTATE_IN_UI && gMainHandler)
		if(memcmp( &rules, &tmpGame->rules, sizeof (class RulesClass) ) )
			dirty = TRUE;

	// Copy in new data
	memcpy (&gameType, &tmpGame->gameType, sizeof (FalconGameType));
	memcpy (&rules, &tmpGame->rules, sizeof (class RulesClass));

	// Attach any sessions not in our list
	ent = iter.GetFirst();
	while (ent){
#if VU_ALL_FILTERED
		if (!sessionCollection_->Find(ent)){
			AddSession(ent);
		}
#else
		if (!sessionCollection_->Find(ent->Id())){
			AddSession(ent);
		}
#endif
		ent = iter.GetNext();
	}

	if(dirty)
		PostMessage(FalconDisplay.appWin,FM_UPDATE_RULES,NULL,NULL);

	// Let vu handle the rest.
	return (VuGameEntity::Handle(event));
}

// KCK: This is the function which determines what we do with all
// of a session's entities when it's gone off-line
VU_ERRCODE FalconGameEntity::Distribute(VuSessionEntity *sess)
{
	FalconSessionEntity	*new_host;

	MonoPrint("Calling Distribute() for %s",gameName_);
	if (sess)
		MonoPrint("- player: %s.\n",((FalconSessionEntity*)sess)->GetPlayerCallsign());
	else
		MonoPrint("- distribute all.\n");

	// KCK: Try using association to let VU do the distribution for us!
	VuGameEntity::Distribute(sess);

	new_host = (FalconSessionEntity*)vuDatabase->Find(OwnerId());
	if (new_host)
		MonoPrint("New Host: %s\n",((FalconSessionEntity*)new_host)->GetPlayerCallsign());
	else
		MonoPrint("No new host - shutting game down\n");

/*	if (!sess || Id() == sess->GameId()) 
		{
		// All Features get transfered to nearest session.
		// All Non-player transferable vehicles (i.e. non-dogfight) get transfered to nearest session.
		// Everything else gets removed
		VuDatabaseIterator	dbiter;
		VuEntity			*ent;
		uchar				ent_class;
		FalconSessionEntity	*new_host = (FalconSessionEntity*)vuDatabase->Find(OwnerId());

		// Step 1) A new game owner is picked, if necessary - basically, the session with the lowest creator id.
		if (new_host == sess)
			{
			VuSessionsIterator	siter(this);
			FalconSessionEntity	*cs;
			ulong				best = ~0;
		
			new_host = NULL;
			cs = (FalconSessionEntity*) siter.GetFirst();
			while (cs) 
				{
				if ((!sess || sess->Id() != cs->Id()) && cs->GameId() == Id() && (ulong)cs->Id().creator_ < best) 
					{
					best = cs->Id().creator_;
					new_host = cs;
					}
				cs = (FalconSessionEntity*) siter.GetNext();
				}
			}

		// We must be loaded in order to take over this game
		if (new_host == FalconLocalSession && !TheCampaign.IsLoaded())
			CampaignJoinFail(TRUE);

		if (new_host)
			MonoPrint("New Host: %s\n",((FalconSessionEntity*)new_host)->GetPlayerCallsign());
		else
			MonoPrint("No new host - shutting game down\n");

		// Step 2: Redistribute all the entities
		// The Game and any Groups get transfered to game owner.
		// All Objectives get transfered to game owner.
		// All Units get transfered to game owner.
		// All Managers get transfered to game owner.
		// All Transferable Features and Vehicles are temporarily transfered to the game owner 
		// - but should get transfered or reaggregated when the new owner handles the reaggregate message for the parent.
		// All other entities are removed.
		VuEnterCriticalSection();
		ent = dbiter.GetFirst();
		while (ent) 
			{
			if ((!sess || ent->OwnerId() == sess->Id()) && ent != sess)
				{
				ent_class = ent->EntityType()->classInfo_[VU_CLASS];
				if (!new_host || (sess && !ent->IsTransferrable()))
					{
					vuDatabase->Remove(ent);
					}
				else if (ent_class == CLASS_OBJECTIVE || ent_class == CLASS_UNIT || ent_class == CLASS_MANAGER || ent_class == CLASS_GAME || ent_class == CLASS_GROUP)
					{
					((FalconEntity*)ent)->SetOwner(new_host);
					((CampEntity)ent)->SendMessage(sess->Id(), FalconCampMessage.campReaggregate, 0, 0, 0, 0);
					}
				else if (ent_class == CLASS_FEATURE || ent_class == CLASS_VEHICLE)
					{
					// KCK: If there's a valid deaggregate owner, set owner to that..
					if ((int)((SimBaseClass*)ent)->campaignObject > MAX_IA_CAMP_UNIT && ((CampBaseClass*)((SimBaseClass*)ent)->campaignObject)->deag_owner != sess->Id())
						((SimBaseClass*)ent)->ChangeOwner(((CampBaseClass*)((SimBaseClass*)ent)->campaignObject)->GetDeaggregateOwner());
					// Otherwise, change ownership to the host (let campaign entity know too!)
					else
						{
						((SimBaseClass*)ent)->ChangeOwner(new_host);
						((CampBaseClass*)((SimBaseClass*)ent)->campaignObject)->deag_owner = new_host->Id();
						}
					}
				else
					{
					vuDatabase->Remove(ent);
					}
				}
			ent = dbiter.GetNext();
			}
		VuExitCriticalSection();
		}
*/
	if( FalconLocalGame && FalconLocalSession && \
			FalconLocalSession->GetFlyState() == FLYSTATE_IN_UI && \
			FalconLocalGame->OwnerId() == FalconLocalSession->Id() && \
			gMainHandler)
		{
			INFOSetupRulesControls();
		}

	return VU_SUCCESS;
	}
