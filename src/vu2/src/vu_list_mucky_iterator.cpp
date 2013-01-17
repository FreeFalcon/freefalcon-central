#if 0
#include "vu2.h"
#include "vu_priv.h"

//-----------------------------------------------------------------------------
// VuListMuckyIterator
//-----------------------------------------------------------------------------

VuListMuckyIterator::VuListMuckyIterator(VuLinkedList *coll) : VuIterator(coll){
	curr_ = vuTailNode;
	last_ = 0;
//	vuCollectionManager->Register(this);
}

VuListMuckyIterator::~VuListMuckyIterator(){
//	vuCollectionManager->DeRegister(this);
}

void VuListMuckyIterator::InsertCurrent(VuEntity *ent){
	if (curr_ != ((VuLinkedList*)collection_)->head_){
		// ensure that last_ is not deleted
		if (last_ == 0 || last_->freenext_ != 0){
			last_ = ((VuLinkedList*)collection_)->head_;
			while (last_->entity_ && last_->next_ != curr_){
				last_ = last_->next_;
			}
		}
		if (curr_->freenext_ != 0){
			curr_ = last_->next_; 
		}
		last_->next_ = new VuLinkNode(ent, curr_);
	}
	else {
		((VuLinkedList*)collection_)->head_ = new VuLinkNode(ent, ((VuLinkedList *)collection_)->head_);
	}
}

void VuListMuckyIterator::RemoveCurrent(){
	if (curr_->freenext_ != 0){
		// already done
		return;
	}
	if (curr_ != ((VuLinkedList*)collection_)->head_){
		// ensure that last_ is not deleted
		if (last_ == 0 || last_->freenext_ != 0){
			last_ = ((VuLinkedList *)collection_)->head_;
			while (last_->entity_ && last_->next_ != curr_){
				last_ = last_->next_;
			}
		}
		last_->next_ = curr_->next_;
	}
	else {
		((VuLinkedList*)collection_)->head_ = curr_->next_;
	}
	// put curr on VUs pending delete queue
	// get next before doing it!
	VuLinkNode *to_be_killed = curr_;
	curr_ = curr_->next_;
	vuCollectionManager->PutOnKillQueue(to_be_killed);
}

VuEntity *VuListMuckyIterator::CurrEnt(){
	return curr_->entity_.get();
}

/*VU_BOOL VuListMuckyIterator::IsReferenced(VuEntity* ent){
	// 2002-02-04 MODIFIED BY S.G. 
	// If ent is false, then it can't be a valid entity, right? That's what I think too :-)
	//	if ((last_ && last_->entity_ == ent) || curr_->entity_ == ent)
	if (ent && ((last_ && last_->entity_.get() == ent) || curr_->entity_.get() == ent)){
		return TRUE;
	}
	else {
		return FALSE;
	}
}*/

VU_ERRCODE VuListMuckyIterator::Cleanup(){
	curr_ = vuTailNode;
	return VU_SUCCESS;
}

#endif