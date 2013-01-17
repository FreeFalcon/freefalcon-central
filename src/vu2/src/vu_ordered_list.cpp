#include "vu2.h"
#include "vu_priv.h"

#if VU_ALL_FILTERED

VuOrderedList::VuOrderedList(VuFilter* filter) : VuLinkedList(filter)
{}

VuOrderedList::~VuOrderedList()
{}

VU_ERRCODE VuOrderedList::PrivateInsert(VuEntity* entity){
	for (VuLinkedList::iterator it=l_.begin();it!=l_.end();++it){
		VuEntityBin &b = *it;
		if (GetFilter()->Compare(*b, entity) >= 0) {
			l_.insert(it, VuEntityBin(entity));
			return VU_SUCCESS;
		}
	}
	l_.push_back(VuEntityBin(entity));
	return VU_SUCCESS;
}

VU_ERRCODE VuOrderedList::PrivateRemove(VuEntity* entity){
	for (VuLinkedList::iterator it=l_.begin();it!=l_.end();++it){
		VuEntityBin &b = *it;
		int res = GetFilter()->Compare(*b, entity);
		if (res == 0){
			l_.erase(it);
			return VU_SUCCESS;
		}
		else if (res > 0){
			// all other elements are smaller than entity, can stop
			return VU_NO_OP;
		}
	}
	return VU_SUCCESS;
}

bool VuOrderedList::PrivateFind(VuEntity* entity){
	for (VuLinkedList::iterator it=l_.begin();it!=l_.end();++it){
		VuEntityBin &b = *it;
		int res = GetFilter()->Compare(*b, entity);
		if (res == 0){
			return true;
		}
		else if (res > 0){
			// all other elements are smaller than entity, can stop
			return false;
		}
	}
	return false;
}

#else

VuOrderedList::VuOrderedList(VuFilter* filter) : VuFilteredList(filter)
{}

VuOrderedList::~VuOrderedList()
{}


VU_ERRCODE VuOrderedList::ForcedInsert(VuEntity* entity){
	if (entity == NULL){ return VU_NO_OP; }

	VuScopeLock l(GetMutex());
	if (filter_->RemoveTest(entity)) {
		for (VuLinkedList::iterator it=l_.begin();it!=l_.end();++it){
			VuEntityBin &b = *it;
			if (filter_->Compare(*b, entity) >= 0) {
				l_.insert(it, VuEntityBin(entity));
				return VU_SUCCESS;
			}
		}
		l_.push_back(VuEntityBin(entity));
		return VU_SUCCESS;
	}
	return VU_NO_OP;
}

VU_ERRCODE VuOrderedList::Insert(VuEntity *entity){
	VuScopeLock l(GetMutex());
	if (filter_->Test(entity)){
		return ForcedInsert(entity);
	}
	return VU_NO_OP;
}

#endif

VU_COLL_TYPE VuOrderedList::Type() const {
	return VU_ORDERED_LIST_COLLECTION;
}
