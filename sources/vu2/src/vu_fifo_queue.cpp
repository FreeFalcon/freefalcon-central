#if 0
// sfr: not used
#include "vu2.h"
#include "vu_priv.h"

VuFifoQueue::VuFifoQueue(VuFilter* filter) : VuFilteredList(filter){
//	last_ = head_;
}

VuFifoQueue::~VuFifoQueue(){
}

VU_ERRCODE VuFifoQueue::Insert(VuEntity* entity){
	VuScopeLock l(GetMutex());
	if (filter_->Test(entity)){
		return ForcedInsert(entity);
	}
	return VU_NO_OP;
}

VU_ERRCODE VuFifoQueue::ForcedInsert(VuEntity* entity){
	VuScopeLock l(GetMutex());
	if (filter_->RemoveTest(entity)){
		VuLinkedList::Insert(entity);
#if 0
		if (head_ != tail_) {
			last_->next_ = new VuLinkNode(entity, tail_);
			last_        = last_->next_;
		} 
		else {
			head_ = last_ = new VuLinkNode(entity, tail_); // first entity in queue...
		}
#endif
		return VU_SUCCESS;
	}
	return VU_NO_OP;
}

VU_ERRCODE VuFifoQueue::Remove(VuEntity *entity){
	return VuFilteredList::Remove(entity);
#if 0
	VuScopeLock l(GetMutex());
	VuLinkNode* cur = head_;
	VuLinkNode* last = 0;
	// run all in fifo
	while (cur->entity_){
		if (cur->entity_.get() == entity){
			// found the entity
			if (last){
				// here we are not in first position
				last->next_ = cur->next_;
			}
			else{
				// new head
				head_ = cur->next_;
			}
			// fifo tail, need to create a new tail
			if (cur == last_){
				if (last){
					last_ = last;
				}
				else{
					last_ = head_;
				}
			}
			vuCollectionManager->PutOnKillQueue(cur);
			return VU_SUCCESS;
		}
		last = cur;
		cur  = cur->next_;
	}
	return VU_NO_OP;
#endif
}

VU_ERRCODE VuFifoQueue::Remove(VU_ID entityId){
	return VuFilteredList::Remove(entityId);
#if 0
	VuEntity* ent = vuDatabase->Find(entityId);
	if (ent){
		return VuFifoQueue::Remove(ent);
	}
	return VU_NO_OP;
#endif
}

int VuFifoQueue::Purge(VU_BOOL all){
	return VuFilteredList::Purge(all);
#if 0
	VuScopeLock l(GetMutex());
	int retval = VuLinkedList::Purge(all);
	last_ = head_;
	while (last_->entity_){
		last_ = last_->next_;
	}
	return retval;
#endif
}

VuEntity *VuFifoQueue::Peek(){
	VuScopeLock l(GetMutex());
	return l_.empty() ? NULL : l_.back().get();
//	return head_->entity_.get();
}

VuEntity *VuFifoQueue::Pop(){
	VuScopeLock l(GetMutex());
	if (l_.empty()){ return NULL; }
	VuEntityBin eb = l_.back();
	l_.pop_back();
	return eb.get();
#if 0
	VuEntity *retval = Peek();
	Remove(retval);
	return retval;
#endif
}

VU_COLL_TYPE VuFifoQueue::Type() const {
	return VU_FIFO_QUEUE_COLLECTION;
}
#endif

