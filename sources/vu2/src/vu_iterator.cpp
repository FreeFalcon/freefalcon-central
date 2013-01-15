#include "vu2.h"
#include "vu_priv.h"

VuIterator::VuIterator(VuCollection *coll) : collection_(coll){
	if (collection_ != NULL){
		VuMutex m = collection_->GetMutex();
		if (m != NULL){
			VuxLockMutex(m);
		}
	}
}

VuIterator::~VuIterator(){
	if (collection_ != NULL){
		VuMutex m = collection_->GetMutex();
		if (m != NULL){
			VuxUnlockMutex(m);
		}
	}
}

VU_ERRCODE VuIterator::Cleanup(){
	// by default, do nothing
	return VU_SUCCESS;
}
