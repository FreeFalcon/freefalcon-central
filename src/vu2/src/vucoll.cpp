#include "vu2.h"
#include "vu_priv.h"


#if VU_ALL_FILTERED
VuCollection::VuCollection(VuFilter *filter, bool threadSafe) : 
	mutex_(threadSafe ? VuxCreateMutex("collection mutex") : NULL), 
	filter_(filter == NULL ? NULL : filter->Copy()),
	registered(false)
{
}

VuCollection::~VuCollection(){
	if (registered){ Unregister(); }
	
	if (filter_){ delete filter_; filter_ = NULL; }
	if (mutex_){ VuxDestroyMutex(mutex_); }
}

void VuCollection::Register(){
	if (!registered){
		vuCollectionManager->Register(this);
		registered = true;
	}
}


void VuCollection::Unregister(){
	if (registered){
		vuCollectionManager->DeRegister(this);
		registered = false;
	}
}

VU_ERRCODE VuCollection::Handle(VuMessage *msg){
	if (!filter_){
		return VU_NO_OP;
	}
	else {
		if (filter_->Notice(msg)){
			VuEntity *ent = msg->Entity();
			if (ent && filter_->RemoveTest(ent)){
				if (Find(ent)){
					if (!filter_->Test(ent)){
						// ent is in table, but shouldnt
						PrivateRemove(ent);
					}
				}
				else if (filter_->Test(ent)){
					// ent is not in table, but should be in.
					PrivateInsert(ent);
				}
				return VU_SUCCESS;
			}
		}
		return VU_NO_OP;
	}
}

VU_ERRCODE VuCollection::ForcedInsert(VuEntity *entity){
	if (entity == NULL){ return VU_NO_OP; }
	return PrivateInsert(entity);
}

VU_ERRCODE VuCollection::Insert(VuEntity *entity){
	if (entity == NULL){ return VU_NO_OP; }
	if (filter_ == NULL){ return PrivateInsert(entity); }
	VuScopeLock l(GetMutex());
	// must pass test
	if (filter_->Test(entity)){
		return PrivateInsert(entity);
	}
	return VU_NO_OP;
}

VU_ERRCODE VuCollection::Remove(VuEntity *entity){
	if (entity == NULL){ return VU_NO_OP; }
	if (filter_ == NULL){ return PrivateRemove(entity); }
	VuScopeLock l(GetMutex());
	if (filter_->RemoveTest(entity)){
		return PrivateRemove(entity);
	}
	return VU_NO_OP;
}

bool VuCollection::Find(VuEntity *entity) const {
	if (entity == NULL){ return false; }
	if ((filter_ != NULL) && !filter_->Test(entity)){ return false; }
	return PrivateFind(entity);
}

VuFilter *VuCollection::GetFilter() const {
	return const_cast<VuFilter*>(filter_);
}

#else
VuCollection::VuCollection(bool threadSafe) : 
	mutex_(threadSafe ? VuxCreateMutex("collection mutex") : NULL), 
	registered(false)
{
}

VuCollection::~VuCollection(){
	if (registered){
		Unregister();
	}
	if (mutex_){
		VuxDestroyMutex(mutex_);
	}
}

void VuCollection::Register(){
	vuCollectionManager->Register(this);
	registered = true;
}

void VuCollection::Unregister(){
	vuCollectionManager->DeRegister(this);
	registered = false;
}

VU_ERRCODE VuCollection::Handle(VuMessage *){
	return VU_NO_OP;
}

#endif
