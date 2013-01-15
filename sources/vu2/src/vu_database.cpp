#include "vu2.h"
#include "vu_priv.h"

/** vu database global definition. */
VuDatabase *vuDatabase = 0;

#if VU_ALL_FILTERED

VuDatabase::VuDatabase(unsigned int tableSize, unsigned int key)
	: dbHash_(new VuHashTable(NULL, tableSize, key))
{
}

VuDatabase::~VuDatabase(){
	Purge(TRUE);
}

unsigned int VuDatabase::Purge(VU_BOOL all){
	// this is a bit different from hash db since we need to save entities before purging
	// to avoid self destruction CTD (entity is destroyed and its callback removes other entities
	// from vudb
	// suspend calls removal callback
	std::list<VuEntityBin> toBePurged;
	int ret = 0;
	for (unsigned int i=0;i< dbHash_->capacity_;++i){
		VuListIterator li(&dbHash_->table_[i]);
		VuEntity *e;
		for (
			e = li.GetFirst();
			e != NULL;
			e = li.GetNext()
		){
			// run calling all callbacks and seting as removed... purge will do the actual removal
			if (!(!all && ((e->IsPrivate() && e->IsPersistent()) || e->IsGlobal()))){
				toBePurged.push_back(VuEntityBin(e));
			}
		}
	}
	ret = dbHash_->Purge(all);
	// now delete all
	toBePurged.clear();
	return ret;
}

int VuDatabase::Suspend(VU_BOOL all){
	// suspend calls removal callback
	int ret = 0;
	// similar to purge here...
	std::list<VuEntityBin> toBeSuspended;
	for (unsigned int i=0;i< dbHash_->capacity_;++i){
		VuListIterator li(&dbHash_->table_[i]);
		VuEntity *e;
		for (
			e = li.GetFirst();
			e != NULL;
			e = li.GetNext()
		){
			// run calling all callbacks and seting as removed... purge will do the actual removal
			if (!(!all && ((e->IsPrivate() && e->IsPersistent()) || e->IsGlobal()))){
				toBeSuspended.push_back(VuEntityBin(e));
				e->RemovalCallback();
				e->SetVuState(VU_MEM_REMOVED);
			}
		}
	}
	ret = dbHash_->Purge(all);
	// now remove all all
	toBeSuspended.clear();
	return ret;
}

VU_ERRCODE VuDatabase::Handle(VuMessage *msg){
	// note: this should work on Create & Delete messages, but those are
	// currently handled elsewhere... for now... just pass on to collection mgr
	return vuCollectionManager->Handle(msg);
}

void VuDatabase::HandleMove(VuEntity *ent, BIG_SCALAR coord1, BIG_SCALAR coord2){
	if (ent->VuState() == VU_MEM_ACTIVE){
		vuCollectionManager->HandleMove(ent, coord1, coord2);
	}
}

VU_ERRCODE VuDatabase::Insert(VuEntity *entity){
	if (entity == NULL){ return VU_NO_OP; }

#if 0//BIRTH_LIST
	// already in
	if ((entity->VuState() == VU_MEM_ACTIVE) || (entity->VuState() == VU_MEM_TO_BE_INSERTED)){
		return VU_ERROR;
	}

	VuScopeLock l(GetMutex());
	entity->SetVuState(VU_MEM_TO_BE_INSERTED);
	vuCollectionManager->AddToBirthList(entity);
	VuEntity::VU_SEND_TYPE sendType = entity->SendCreate();
	if (entity->IsLocal() && (!entity->IsPrivate()) && (sendType != VuEntity::VU_SC_DONT_SEND)) {
		VuCreateEvent *event = 0;
		VuTargetEntity *target = vuGlobalGroup;
		if (!entity->IsGlobal()) {
			target = vuLocalSessionEntity->Game();
		}
		event = new VuCreateEvent(entity, target);
		event->RequestReliableTransmit();
		if (sendType == VuEntity::VU_SC_SEND_OOB){
			event->RequestOutOfBandTransmit();
		}
		VuMessageQueue::PostVuMessage(event);
	}
	entity->InsertionCallback();
	return VU_SUCCESS;

#else 
	// no duplicates allowed
	if ((entity->VuState() == VU_MEM_ACTIVE) || (dbHash_->Find(entity->Id()) != NULL)){
		return VU_ERROR;
	}

	entity->SetVuState(VU_MEM_ACTIVE);
	dbHash_->Insert(entity);
	vuCollectionManager->Add(entity);
	
	VuEntity::VU_SEND_TYPE sendType = entity->SendCreate();
	if (entity->IsLocal() && (!entity->IsPrivate()) && (sendType != VuEntity::VU_SC_DONT_SEND)) {
		VuCreateEvent *event = 0;
		VuTargetEntity *target = vuGlobalGroup;
		if (!entity->IsGlobal()) {
			target = vuLocalSessionEntity->Game();
		}
		event = new VuCreateEvent(entity, target);
		event->RequestReliableTransmit();
		if (sendType == VuEntity::VU_SC_SEND_OOB){
			event->RequestOutOfBandTransmit();
		}
		VuMessageQueue::PostVuMessage(event);
	}

	// sfr: its possible this is being called twice (because of the create event)
	entity->InsertionCallback();
	return VU_SUCCESS;
#endif
}

#define IMMEDIATE_REMOVAL_CALLBACK 0

VU_ERRCODE VuDatabase::CommonRemove(VuEntity *entity){
	if (!entity || entity->VuState() != VU_MEM_ACTIVE){ return VU_NO_OP; }
	vuCollectionManager->AddToGc(entity);
	entity->SetVuState(VU_MEM_INACTIVE);

#if IMMEDIATE_REMOVAL_CALLBACK
	entity->RemovalCallback();
#endif

	return VU_SUCCESS;
}

VU_ERRCODE VuDatabase::Remove(VuEntity *entity){
	VU_ERRCODE ret = CommonRemove(entity);
	if (ret != VU_SUCCESS){ return ret; }

#if NO_RELEASE_EVENT
	if (entity->IsLocal() && !entity->IsPrivate()) {
		VuEvent *event = new VuDeleteEvent(entity);
		event->RequestReliableTransmit();
		VuMessageQueue::PostVuMessage(event);
	} 
#else
	VuEvent *event;
	if (entity->IsLocal() && !entity->IsPrivate()) {
		event = new VuDeleteEvent(entity);
		event->RequestReliableTransmit();
	} 
	else {
		event = new VuReleaseEvent(entity);
	}
	VuMessageQueue::PostVuMessage(event);
#endif
	return VU_SUCCESS;
}

VU_ERRCODE VuDatabase::SilentRemove(VuEntity *entity){
	return CommonRemove(entity);
}

#if !NO_RELEASE_EVENT
VU_ERRCODE VuDatabase::DeleteRemove(VuEntity *entity){
	return CommonRemove(entity);
}
#endif

VU_ERRCODE VuDatabase::Remove(VU_ID entityId){
	return Remove(dbHash_->Find(entityId));
}

#if BIRTH_LIST
void VuDatabase::ReallyInsert(VuEntity *entity){
	entity->SetVuState(VU_MEM_ACTIVE);
	dbHash_->Insert(entity);
	vuCollectionManager->Add(entity);
}
#endif

void VuDatabase::ReallyRemove(VuEntity *entity){
	// play it safe
	VuBin<VuEntity> safe(entity);
#if !IMMEDIATE_REMOVAL_CALLBACK
	entity->RemovalCallback();
#endif
	dbHash_->Remove(entity);
	vuCollectionManager->Remove(entity);
	entity->SetVuState(VU_MEM_REMOVED);
}

VuEntity *VuDatabase::Find(VU_ID entityId) const {
	return dbHash_->Find(entityId);
}

#else

VuDatabase::VuDatabase(unsigned int tableSize, unsigned int key)
	: VuHashTable(tableSize, key)
{
}

VuDatabase::~VuDatabase(){
	Purge(TRUE);
}

unsigned int VuDatabase::Purge(VU_BOOL all){
	unsigned int ret = 0;

	for (unsigned int i=0;i<capacity_;++i){
		VuListIterator li(&table_[i]);
		VuEntity *e;
		for (
			e = li.GetFirst();
			e != NULL;
			e = li.GetNext()
		){
			if (!(!all && ((e->IsPrivate() && e->IsPersistent()) || e->IsGlobal()))){
				e->SetVuState(VU_MEM_REMOVED);
			}
		}
		ret += table_[i].Purge(all);
	}
	return ret;
}

int VuDatabase::Suspend(VU_BOOL all){
	// suspend calls removal callback
	int ret = 0;
	for (unsigned int i=0;i<capacity_;++i){
		VuListIterator li(&table_[i]);
		VuEntity *e;
		for (
			e = li.GetFirst();
			e != NULL;
			e = li.GetNext()
		){
			if (!(!all && ((e->IsPrivate() && e->IsPersistent()) || e->IsGlobal()))){
				e->RemovalCallback();
				e->SetVuState(VU_MEM_REMOVED);
			}
		}
		ret += table_[i].Purge(all);
	}
	return ret;
}

VU_ERRCODE VuDatabase::Handle(VuMessage *msg){
	// note: this should work on Create & Delete messages, but those are
	// currently handled elsewhere... for now... just pass on to collection mgr
	vuCollectionManager->Handle(msg);
	return VU_SUCCESS;
}

void VuDatabase::HandleMove(VuEntity *ent, BIG_SCALAR coord1, BIG_SCALAR coord2){
	if (ent->VuState() == VU_MEM_ACTIVE){
		vuCollectionManager->HandleMove(ent, coord1, coord2);
	}
}

VU_ERRCODE VuDatabase::Insert(VuEntity *entity){
	if (entity == NULL){ return VU_NO_OP; }

#if 0//BIRTH_LIST
	// already in
	if ((entity->VuState() == VU_MEM_ACTIVE) || (entity->VuState() == VU_MEM_TO_BE_INSERTED)){
		return VU_ERROR;
	}

	VuScopeLock l(GetMutex());
	entity->SetVuState(VU_MEM_TO_BE_INSERTED);
	vuCollectionManager->AddToBirthList(entity);
	VuEntity::VU_SEND_TYPE sendType = entity->SendCreate();
	if (entity->IsLocal() && (!entity->IsPrivate()) && (sendType != VuEntity::VU_SC_DONT_SEND)) {
		VuCreateEvent *event = 0;
		VuTargetEntity *target = vuGlobalGroup;
		if (!entity->IsGlobal()) {
			target = vuLocalSessionEntity->Game();
		}
		event = new VuCreateEvent(entity, target);
		event->RequestReliableTransmit();
		if (sendType == VuEntity::VU_SC_SEND_OOB){
			event->RequestOutOfBandTransmit();
		}
		VuMessageQueue::PostVuMessage(event);
	}
	entity->InsertionCallback();
	return VU_SUCCESS;

#else 
	// no duplicates allowed
	if ((entity->VuState() == VU_MEM_ACTIVE) || (Find(entity->Id()) != NULL)){
		return VU_ERROR;
	}

	VuScopeLock lock(GetMutex());

	entity->SetVuState(VU_MEM_ACTIVE);
	VuHashTable::Insert(entity);
	vuCollectionManager->Add(entity);
	
	VuEntity::VU_SEND_TYPE sendType = entity->SendCreate();
	if (entity->IsLocal() && (!entity->IsPrivate()) && (sendType != VuEntity::VU_SC_DONT_SEND)) {
		VuCreateEvent *event = 0;
		VuTargetEntity *target = vuGlobalGroup;
		if (!entity->IsGlobal()) {
			target = vuLocalSessionEntity->Game();
		}
		event = new VuCreateEvent(entity, target);
		event->RequestReliableTransmit();
		if (sendType == VuEntity::VU_SC_SEND_OOB){
			event->RequestOutOfBandTransmit();
		}
		VuMessageQueue::PostVuMessage(event);
	}

	// sfr: its possible this is being called twice (because of the create event)
	entity->InsertionCallback();
	return VU_SUCCESS;
#endif
}

VU_ERRCODE VuDatabase::CommonRemove(VuEntity *entity){
	if (!entity || entity->VuState() != VU_MEM_ACTIVE){ return VU_NO_OP; }
	VuScopeLock l(GetMutex());
	vuCollectionManager->AddToGc(entity);
	entity->SetVuState(VU_MEM_INACTIVE);
	entity->RemovalCallback();
	return VU_SUCCESS;
}

VU_ERRCODE VuDatabase::Remove(VuEntity *entity){
	VU_ERRCODE ret = CommonRemove(entity);
	if (ret != VU_SUCCESS){ return ret; }

	VuEvent *event;
	if (entity->IsLocal() && !entity->IsPrivate()) {
		event = new VuDeleteEvent(entity);
		event->RequestReliableTransmit();
	} 
#if !NO_RELEASE_EVENT
	else {
		event = new VuReleaseEvent(entity);
	}
#endif
	VuMessageQueue::PostVuMessage(event);
	return VU_SUCCESS;
}

VU_ERRCODE VuDatabase::SilentRemove(VuEntity *entity){
	return CommonRemove(entity);
}

#if !NO_RELEASE_EVENT
VU_ERRCODE VuDatabase::DeleteRemove(VuEntity *entity){
	return CommonRemove(entity);
}
#endif

VU_ERRCODE VuDatabase::Remove(VU_ID entityId){
	return Remove(Find(entityId));
}

#if BIRTH_LIST
void VuDatabase::ReallyInsert(VuEntity *entity){
	entity->SetVuState(VU_MEM_ACTIVE);
	VuHashTable::Insert(entity);
	vuCollectionManager->Add(entity);
}
#endif

void VuDatabase::ReallyRemove(VuEntity *entity){
	// play it safe
	VuBin<VuEntity> safe(entity);
	VuHashTable::Remove(entity);
	vuCollectionManager->Remove(entity);
	entity->SetVuState(VU_MEM_REMOVED);
}

#endif