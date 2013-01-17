#include "vu2.h"

#if VU_ALL_FILTERED

VuRedBlackTree::VuRedBlackTree(VuKeyFilter *filter) : VuCollection(filter)
{}

VuRedBlackTree::~VuRedBlackTree(){
	Unregister();
	Purge(TRUE);
}

VU_ERRCODE VuRedBlackTree::PrivateInsert(VuEntity* entity){
	VuKeyFilter *kf = GetKeyFilter();
	map_.insert(std::make_pair(kf->Key(entity), VuEntityBin(entity)));
	return VU_SUCCESS;
}

VU_ERRCODE VuRedBlackTree::PrivateRemove(VuEntity* entity){
	VuKeyFilter *kf = GetKeyFilter();
	VU_KEY k = kf->Key(entity);
	unsigned int count = map_.count(k);
	if (count == 0){ return VU_NO_OP; }
	RBMap::iterator it = map_.find(k);
	do {
		VuEntity *e = it->second.get();
		if (e == entity){
			map_.erase(it);
			return VU_SUCCESS;
		}
		++it;
	} while (--count);
	return VU_NO_OP;
}

bool VuRedBlackTree::PrivateFind(VuEntity *ent) const {
	VuKeyFilter *kf = GetKeyFilter();
	VU_KEY k = kf->Key(ent);
	// get number of entities sharing key
	unsigned int count = map_.count(k);
	if (count == 0){ return false; }
	// see which one is the one we want
	RBMap::const_iterator it = map_.find(k);
	do {
		VuEntityBin eb = it->second;
		if ((eb->VuState() == VU_MEM_ACTIVE) && (eb.get() == ent)){
			return true;
		}
		++it;
	} while (--count);
	// can happen if all entities with given key are inactive
	return false;
}


unsigned int VuRedBlackTree::Purge(VU_BOOL all){
	unsigned int ret = 0;
	for (
		RBMap::iterator it = map_.begin();
		it != map_.end();
	){
		VuEntityBin &eb = it->second;
		if (all || (!(eb->IsPrivate() && eb->IsPersistent()) && !eb->IsGlobal())){ 
			it = map_.erase(it);
			++ret;
		}
		else {
			++it;
		}
	}
	return ret;
}

unsigned int VuRedBlackTree::Count() const {
	int count = 0;
	for (
		RBMap::const_iterator it = map_.begin();
		it != map_.end();
		++it
	){
		VuEntityBin eb = it->second;
		if (eb->VuState() == VU_MEM_ACTIVE){
			++count;
		}
	}
	return count;
}

/*VU_KEY VuRedBlackTree::Key(VuEntity *ent) const { 
	VuKeyFilter *kf = GetKeyFilter();
	return kf ? kf->Key(ent) : (VU_KEY)ent->Id(); 
}*/

VuKeyFilter *VuRedBlackTree::GetKeyFilter() const {
	return static_cast<VuKeyFilter*>(GetFilter());
}

VU_COLL_TYPE VuRedBlackTree::Type() const {
	return VU_RED_BLACK_TREE_COLLECTION;
}

#else
VuRedBlackTree::VuRedBlackTree(VuKeyFilter *filter) : 
	VuCollection(), filter_(filter == NULL ? NULL : (VuKeyFilter *)filter->Copy())
{
}

VuRedBlackTree::~VuRedBlackTree(){
	Purge();
	delete filter_;
	filter_ = NULL;
}

VU_ERRCODE VuRedBlackTree::Handle(VuMessage *msg){
	if (filter_ && filter_->Notice(msg)) {
		VuEntity *ent = msg->Entity();
		if (ent && filter_->RemoveTest(ent)){
			if (Find(ent)){
				if (!filter_->Test(ent)) {
					// ent is in table, but doesn't belong there...
					Remove(ent);
				}
			} else {
				// ent is not in table, but does belong there...
				Insert(ent);
			}
			return VU_SUCCESS;
		}
	}
	return VU_NO_OP;
}

VU_ERRCODE VuRedBlackTree::ForcedInsert(VuEntity* entity){
	if (entity == NULL){ return VU_NO_OP; }
	VuScopeLock l(GetMutex());
	if (!filter_->RemoveTest(entity)){ 
		return VU_NO_OP; 
	}
	map_.insert(std::make_pair(Key(entity), VuEntityBin(entity)));
	return VU_SUCCESS;
}

VU_ERRCODE VuRedBlackTree::Insert(VuEntity* entity){
	if (entity == NULL){ return VU_NO_OP; }
	VuScopeLock l(GetMutex());
	if (!filter_->Test(entity)) {
		return VU_NO_OP;
	}
	return ForcedInsert(entity);
}

VU_ERRCODE VuRedBlackTree::Remove(VuEntity* entity){
	if (entity == NULL){ return VU_NO_OP; }
	VuScopeLock l(GetMutex());
	if (!filter_->RemoveTest(entity)){
		return VU_NO_OP;
	}
	
	VU_KEY k = Key(entity);
	unsigned int count = map_.count(k);
	if (count == 0){ return VU_NO_OP; }
	RBMap::iterator it = map_.find(Key(entity));
	do {
		VuEntityBin eb = it->second;
		if (eb.get() == entity){
			map_.erase(it);
			return VU_SUCCESS;
		}
		++it;
	} while (--count);
	return VU_NO_OP;
}

VU_ERRCODE VuRedBlackTree::Remove(VU_ID id){
	return Remove(vuDatabase->Find(id));
}

unsigned int VuRedBlackTree::Purge(VU_BOOL all){
	VuScopeLock l(GetMutex());
	unsigned int ret = 0;
	for (
		RBMap::iterator it = map_.begin();
		it != map_.end();
	){
		VuEntityBin &eb = it->second;
		if (all || (!(eb->IsPrivate() && eb->IsPersistent()) && !eb->IsGlobal())){ 
			it = map_.erase(it);
			++ret;
		}
		else {
			++it;
		}
	}
	return ret;
}

unsigned int VuRedBlackTree::Count() const {
	VuScopeLock l(GetMutex());
	int count = 0;
	for (
		RBMap::const_iterator it = map_.begin();
		it != map_.end();
		++it
	){
		VuEntityBin eb = it->second;
		if (eb->VuState() == VU_MEM_ACTIVE){
			++count;
		}
	}
	return count;
}

VuEntity *VuRedBlackTree::Find(VuEntity *ent) const {
	if (ent == NULL){ return NULL; }
	VuScopeLock l(GetMutex());
	VU_KEY k = Key(ent);
	// get number of entities sharing key
	unsigned int count = map_.count(k);
	if (count == 0){ return NULL; }

	// see which one is the one we want
	RBMap::const_iterator it = map_.find(k);
	do {
		VuEntityBin eb = it->second;
		if ((eb->VuState() == VU_MEM_ACTIVE) && (eb.get() == ent)){
			return ent;
		}
		++it;
	} while (--count);
	// can happen if all entities with given key are inactive
	return NULL;
}

VuEntity *VuRedBlackTree::Find(VU_ID id) const {
	return Find(vuDatabase->Find(id));
}

VU_KEY VuRedBlackTree::Key(VuEntity *ent) const { 
	return filter_ ? filter_->Key(ent) : (VU_KEY)ent->Id(); 
}

VU_COLL_TYPE VuRedBlackTree::Type() const {
	return VU_RED_BLACK_TREE_COLLECTION;
}
#endif
