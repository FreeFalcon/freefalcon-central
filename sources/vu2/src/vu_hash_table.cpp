#include "vu2.h"
#include "vu_priv.h"
#include "vucoll.h"

#if VU_ALL_FILTERED

VuHashTable::VuHashTable(VuFilter *filter, unsigned int tableSize, unsigned int key)
	:  VuCollection(filter), capacity_(tableSize), key_(key)
{
	table_ = new VuLinkedList[capacity_];
}

VuHashTable::~VuHashTable(){
	Unregister();
	Purge(TRUE);
	// this will call lists cleanup
	delete [] table_;
}

VU_ERRCODE VuHashTable::PrivateInsert(VuEntity *entity){
	return table_[getIndex(entity->Id())].ForcedInsert(entity);
}

VU_ERRCODE VuHashTable::PrivateRemove(VuEntity *entity){
	VU_ID eid = entity->Id();
	return table_[getIndex(eid)].Remove(entity);
}

bool VuHashTable::PrivateFind(VuEntity *entity) const {
	return table_[getIndex(entity->Id())].Find(entity);
}


VU_ERRCODE VuHashTable::Remove(VU_ID entityId){
	VuListIterator it(&table_[getIndex(entityId)]);
	for (VuEntity *e = it.GetFirst(); e != NULL; e = it.GetNext()){
		if ((e->VuState() == VU_MEM_ACTIVE) && (e->Id() == entityId)){ 
			it.RemoveCurrent();
			return VU_SUCCESS; 
		}
	}
	return VU_NO_OP;

}

VuEntity *VuHashTable::Find(VU_ID entityId) const {
	VuListIterator it(&table_[getIndex(entityId)]);
	for (VuEntity *e = it.GetFirst(); e != NULL; e = it.GetNext()){
		if ((e->VuState() == VU_MEM_ACTIVE) && (e->Id() == entityId)){ return e; }
	}
	return NULL;
}

#else

VuHashTable::VuHashTable(unsigned int tableSize, unsigned int key)
	:  VuCollection(), capacity_(tableSize), key_(key)
{
	table_ = new VuLinkedList[capacity_];
}

VuHashTable::~VuHashTable(){
	// this will call lists cleanup
	delete [] table_;
}

VU_ERRCODE VuHashTable::Insert(VuEntity *entity){
	if (!entity){
		return VU_NO_OP;
	}

	VuScopeLock l(GetMutex());
	return table_[getIndex(entity->Id())].Insert(entity);
	return VU_SUCCESS;
}

VU_ERRCODE VuHashTable::Remove(VuEntity *entity){
	if (entity == NULL){ return VU_NO_OP; }
	VuScopeLock l(GetMutex());
	VU_ID eid = entity->Id();
	return table_[getIndex(eid)].Remove(entity);
}

VU_ERRCODE VuHashTable::Remove(VU_ID entityId){
	VuScopeLock l(GetMutex());
	return table_[getIndex(entityId)].Remove(entityId);
}

VuEntity *VuHashTable::Find(VU_ID entityId) const {
	VuScopeLock l(GetMutex());
	return table_[getIndex(entityId)].Find(entityId);
}

VuEntity *VuHashTable::Find(VuEntity *ent) const {
	if (ent == NULL){ return NULL; }
	VU_ID eid = ent->Id();
	return table_[getIndex(eid)].Find(eid);
}

#endif

unsigned int VuHashTable::getIndex(VU_ID id) const {
	return ((VU_KEY)id * key_) % capacity_;
}

VU_COLL_TYPE VuHashTable::Type() const {
	return VU_HASH_TABLE_COLLECTION;
}

unsigned int VuHashTable::Purge(VU_BOOL all){
	VuScopeLock l(GetMutex());
	unsigned int ret = 0;
	for (unsigned int i=0;i<capacity_;++i){
		ret += table_[i].Purge(all);
	}
	return ret;
}

unsigned int VuHashTable::Count() const {
	VuScopeLock l(GetMutex());
	unsigned int ret = 0;
	for (unsigned int i=0;i<capacity_;++i){
		ret += table_[i].Count();
	}
	return ret;
}
