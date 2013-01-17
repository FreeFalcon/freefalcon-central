#if 0
// sfr: not used
#include "vu2.h"
#include "vu_priv.h"

VuLifoQueue::VuLifoQueue(VuFilter* filter) : VuFilteredList(filter){
}

VuLifoQueue::~VuLifoQueue(){
}

VU_COLL_TYPE VuLifoQueue::Type() const {
	return VU_LIFO_QUEUE_COLLECTION;
}

VuEntity *VuLifoQueue::Peek(){
	VuScopeLock l(GetMutex());
	return l_.empty() ? NULL : l_.front().get();
#if 0
	return head_->entity_.get();
#endif
}

VuEntity *VuLifoQueue::Pop(){
	VuScopeLock l(GetMutex());
	if (!l_.empty()){
		VuEntity *e = Peek();
		l_.pop_front();
		return e;
	}
	else {
		return NULL;
	}

#if 0
	VuEntity* retval = Peek();
	Remove(retval);
	return retval;
#endif
}
#endif