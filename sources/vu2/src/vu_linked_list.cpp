/** @file vu_linked_list.cpp vu linked list implementation. */

#include "vu2.h"
#include "vu_priv.h"

#if VU_ALL_FILTERED
VuLinkedList::VuLinkedList(VuFilter *filter) : VuCollection(filter)
{}

VuLinkedList::~VuLinkedList(){
	Unregister();
	Purge();
}

VU_ERRCODE VuLinkedList::PrivateInsert(VuEntity* entity){
	VuScopeLock l(GetMutex());
	l_.push_front(VuEntityBin(entity));
	return VU_SUCCESS;
}

VU_ERRCODE VuLinkedList::PrivateRemove(VuEntity* entity){
	VuScopeLock lk(GetMutex());
	for (VuEntityBinList::iterator it = l_.begin(); it != l_.end(); ++it){
		VuEntityBin &eb = *it;
		if (eb.get() == entity){
			l_.erase(it);
			return VU_SUCCESS;
		}
	}
	return VU_NO_OP;
}

bool VuLinkedList::PrivateFind(VuEntity* entity) const {
	VuScopeLock l(GetMutex());
	for (VuEntityBinList::const_iterator it=l_.begin();it!=l_.end();++it){
		VuEntity *e = it->get();
		if (e == entity){ return true; }
	}
	return false;
}

#else
VuLinkedList::VuLinkedList() : VuCollection(){
}

VuLinkedList::~VuLinkedList(){
	Purge(TRUE);
}

VU_ERRCODE VuLinkedList::Insert(VuEntity* entity){
	if (entity == NULL){ return VU_NO_OP; }

	VuScopeLock l(GetMutex());
	l_.push_front(VuEntityBin(entity));
	return VU_SUCCESS;
}


VU_ERRCODE VuLinkedList::Remove(VuEntity* entity){
	if (entity == NULL){ return VU_NO_OP; }

	VuScopeLock lk(GetMutex());
	for (VuEntityBinList::iterator it = l_.begin(); it != l_.end(); ++it){
		VuEntityBin eb = *it;
		if (eb.get() == entity){
			l_.erase(it);
			return VU_SUCCESS;
		}
	}
	return VU_NO_OP;
}


VU_ERRCODE VuLinkedList::Remove(VU_ID eid){
	VuScopeLock lk(GetMutex());
	VU_ERRCODE ret = VU_NO_OP;
	// run removing all with given id
	for (VuEntityBinList::iterator it = l_.begin(); it != l_.end();){
		VuEntityBin &eb = *it;
		if (eb->Id() == eid){
			l_.erase(it);
			ret = VU_SUCCESS;
		}
		else {
			++it;
		}
	}
	return ret;
	
}

VuEntity* VuLinkedList::Find(VuEntity* entity) const {
	if (entity == NULL){ return NULL; }

	VuScopeLock l(GetMutex());
	for (VuEntityBinList::const_iterator it=l_.begin();it!=l_.end();++it){
		VuEntity *e = it->get();
		if (e == entity){ return entity; }
	}
	return NULL;
}

VuEntity* VuLinkedList::Find(VU_ID eid) const {
	VuScopeLock l(GetMutex());
	for (VuEntityBinList::const_iterator it=l_.begin();it!=l_.end();++it){
		VuEntity *e = it->get();
		if (e->Id() == eid && e->VuState() == VU_MEM_ACTIVE){
			return e; 
		}
	}
	return NULL;
}

#endif

unsigned int VuLinkedList::Purge(VU_BOOL all){
	VuScopeLock lk(GetMutex());
	int ret = 0;
	// this is to avoid self destructing CTDs when purging lists
	// like: simentity is removed. its the last from a featgure. feature is destroyed.
	// its component list is removed and its this list... guess what happens
	std::list<VuEntityBin> toBePurged;
	for (VuEntityBinList::iterator it=l_.begin();it!=l_.end();){
		VuEntityBin &ent = *it;
		if (!all && ( ent->IsGlobal() || (ent->IsPrivate() && ent->IsPersistent()) ) ){
			// dont remove global or private pesistant
			++it;
		}
		else {
			toBePurged.push_back(ent);
			it = l_.erase(it);
			++ret;
		}
	}
	toBePurged.clear();
	return ret;
}

unsigned int VuLinkedList::Count() const {
	VuScopeLock l(GetMutex());
	unsigned int count = 0;
	for (VuEntityBinList::const_iterator it=l_.begin();it!=l_.end();++it){
		VuEntity *e = it->get();
		if (e->VuState() == VU_MEM_ACTIVE){ ++count; }
	}
	return count;
}


VU_COLL_TYPE VuLinkedList::Type() const {
	return VU_LINKED_LIST_COLLECTION;
}
