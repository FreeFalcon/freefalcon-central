/** @file vu_mq.cpp message queue implementation */

#include "vu2.h"

////////////////////
// VuMessageQueue //
////////////////////

extern VuMainThread *vuMainThread;

static VuNullMessageFilter vuNullFilter;

VuMessageQueue* VuMessageQueue::queuecollhead_ = 0;

VuMessageQueue::VuMessageQueue(int queueSize, VuMessageFilter* filter){
	head_  = new VuMessage*[queueSize];
	read_  = head_;
	write_ = head_;
	tail_  = head_ + queueSize;

	// initialize queue
	for (int i = 0; i < queueSize; i++) {
		head_[i] = 0;
	}
	if (!filter) {
		filter = &vuNullFilter;
	}
	filter_ = filter->Copy();

	// add this queue to list of queues
	VuEnterCriticalSection();
	nextqueue_     = queuecollhead_;
	queuecollhead_ = this;
	VuExitCriticalSection();
}

VuMessageQueue::~VuMessageQueue()
{
	delete [] head_;
	delete filter_;
	filter_ = 0;

	// delete this queue from list of queues
	VuEnterCriticalSection();
	VuMessageQueue* last = 0;
	VuMessageQueue* cur = queuecollhead_;
	while (cur) {
		if (this == cur) {
			if (last) {
				last->nextqueue_ = this->nextqueue_;
			} 
			else {
				queuecollhead_ = this->nextqueue_;
			}
			// we've removed it... break out of while() loop
			break;
		}
		last = cur;
		cur  = cur->nextqueue_;
	}
	VuExitCriticalSection();
}

VU_BOOL VuMessageQueue::DispatchVuMessage(VU_BOOL autod) {
	// used to return message... not anymore
	VuMessage *msg = 0;
	VuEnterCriticalSection();

	if (*read_) {
		msg = *read_;
		*read_++ = 0;
		if (read_ == tail_) {
			read_ = head_;
		}
		msg->Ref();
		VuExitCriticalSection();
		msg->Dispatch(autod);
		msg->UnRef();
		msg->UnRef();
		return TRUE;
	}
	VuExitCriticalSection();
	return FALSE;
}

int VuMessageQueue::DispatchMessages(int max, VU_BOOL autod){
	if (max == -1){ max = 0x7fffffff; }
	int i = 0;
	while (DispatchVuMessage(autod) && (++i < max)) ;
	return i;
}

int VuMessageQueue::InvalidateQueueMessages(VU_BOOL (*evalFunc)(VuMessage*, void*), void *arg)
{
	VuEnterCriticalSection();
	int         count = 0;
	VuMessage** cur = read_;

	while (*cur) {
		if ((*evalFunc)(*cur, arg) == TRUE) {
			(*cur)->UnRef();
			*cur = new VuUnknownMessage();
			(*cur)->Ref();
			count++;
		}
		*cur++;
		if (cur == tail_) {
			cur = head_;
		}
	}

	VuExitCriticalSection();
	return count;
}
// static functions

// sfr took pseudo bw out
int VuMessageQueue::PostVuMessage(VuMessage* msg) {
	int retval = 0;
	retval = 1;

	// must enter critical section as this modifies multiple threads' queues
	VuEnterCriticalSection();
	msg->Ref(); // Ref/UnRef pair will delete unhandled messages

	// set post time
	if (msg->postTime_ == 0){
		msg->postTime_ = vuxRealTime;
	}

	// activate entity
	VuEntity* ent = msg->Entity();
	if (ent == 0){
		ent = vuDatabase->Find(msg->EntityId());
	}
	msg->Activate(ent);

	if (ent && !msg->IsLocal() && !ent->IsLocal()) {
		ent->SetTransmissionTime(msg->postTime_);
	}

	// outgoing message, try send. If fails, add to send queue
	if (
		vuGlobalGroup && vuGlobalGroup->Connected() &&
		msg->Target() && msg->Target() != vuLocalSessionEntity &&
		msg->DoSend() && (!ent || !ent->IsPrivate()) &&
		(vuLocalSession.creator_ != VU_SESSION_NULL_CONNECTION.creator_)
	){
		retval = msg->Send();
		VuPendingSendQueue *sq = vuMainThread->SendQueue();
		if (
			(retval == 0) && sq &&
			(msg->Flags() & VU_SEND_FAILED_MSG_FLAG) &&
			((msg->Flags() & VU_RELIABLE_MSG_FLAG) || (msg->Flags() & VU_KEEPALIVE_MSG_FLAG)) 
		){
			//if (msg->Flags() & VU_NORMAL_PRIORITY_MSG_FLAG){
				sq->AddMessage(msg);
			//}
			//else{
			//	vuLowSendQueue->AddMessage(msg);				
				//retval = msg->Size();
			//}
			// sfr: why not for both?
			retval = msg->Size();
		}
	}

	// if message is remote or is a local message loopback, place in local queues
	if (
		!msg->IsLocal() ||
		((msg->Flags() & VU_LOOPBACK_MSG_FLAG) && msg->IsLocal())
	){
		VuMessageQueue* cur = queuecollhead_;
		while (cur){
			// sfr: this is only for received messages. exclude send queues
			if (/*cur != vuLowSendQueue && */cur != vuMainThread->SendQueue()){
				cur->AddMessage(msg);
			}
			cur = cur->nextqueue_;
		}
	}
	
	// message not added to any queue, auto destroy
	if (
		(msg->refcnt_ == 1) &&
		(!msg->IsLocal() || (msg->Flags() & VU_LOOPBACK_MSG_FLAG))
	){
		VuExitCriticalSection();
		msg->Process(TRUE);
		vuDatabase->Handle(msg);
		msg->UnRef();
		return retval;
	}
	msg->UnRef();
	VuExitCriticalSection();

	return retval;
}

void VuMessageQueue::FlushAllQueues(){
	// must enter critical section as this modifies multiple threads' queues
	VuEnterCriticalSection();
	for (
		VuMessageQueue* cur = queuecollhead_;
		cur != NULL;
		cur = cur->nextqueue_
	){
		cur->DispatchMessages(-1, TRUE);
	}
	VuExitCriticalSection();
}

/*
int VuMessageQueue::InvalidateMessages(VU_BOOL (*evalFunc)(VuMessage*, void*), void *arg){
	int count = 0;
	VuEnterCriticalSection();
	VuMessageQueue *cur = queuecollhead_;
	while (cur) {
		count += cur->InvalidateQueueMessages(evalFunc, arg);
		cur = cur->nextqueue_;
	}
	VuExitCriticalSection();
	return count;
}
*/

void VuMessageQueue::RepostMessage(VuMessage* msg, int delay){
	msg->flags_ |= ~VU_LOOPBACK_MSG_FLAG;
	VuTimerEvent *timer = new VuTimerEvent(0, vuxRealTime + delay, VU_DELAY_TIMER, msg);
	VuMessageQueue::PostVuMessage(timer);
}

// end statics


VU_BOOL VuMessageQueue::ReallocQueue(){
	int size = (tail_ - head_)*2;
	VuMessage	**newhead, **cp, **rp;

	newhead = new VuMessage*[size];
	cp      = newhead;

	for (rp = read_; rp != tail_; cp++,rp++)
		*cp = *rp;

	for (rp = head_; rp != write_; cp++,rp++)
		*cp = *rp;

	delete[] head_;
	head_  = newhead;
	read_  = head_;
	write_ = cp;
	tail_  = head_ + size;

	while (cp != tail_){
		*cp++ = 0;
	}

	return TRUE;
}

VU_BOOL VuMessageQueue::AddMessage(VuMessage* event){
	// JB 010121
	if (!event || !filter_){
		return 0;
	}

	if (filter_->Test(event) && event->Type() != VU_TIMER_EVENT) {
		event->Ref();
		*write_++ = event;
		if (write_ == tail_) {
			write_ = head_;
		}

		if (write_ == read_ && *read_) {
			if (!ReallocQueue() && write_ == read_ && *read_) {
				// do simple dispatch -- cannot be handled by user
				// danm_note: should we issue a warning here?
				DispatchVuMessage(TRUE);
			}
		}
		return TRUE;
	}
	return FALSE;
}


////////////////////////
// VuMainMessageQueue //
////////////////////////
VuMainMessageQueue::VuMainMessageQueue(int queueSize, VuMessageFilter* filter)
: VuMessageQueue(queueSize, filter){	
	timerlisthead_ = 0;
}

VuMainMessageQueue::~VuMainMessageQueue(){}

VU_BOOL VuMainMessageQueue::DispatchVuMessage(VU_BOOL autod){
	VuEnterCriticalSection();
	while (timerlisthead_ && timerlisthead_->mark_ < vuxRealTime) {
		// message timer mark is older than current timestamp
		VuTimerEvent* oldhead = timerlisthead_;
		timerlisthead_ = timerlisthead_->next_;
		oldhead->Dispatch(autod);
		oldhead->UnRef();
	}
	VuExitCriticalSection();
	return VuMessageQueue::DispatchVuMessage(autod);
}

VU_BOOL VuMainMessageQueue::AddMessage(VuMessage* msg){
	if (msg->Type() == VU_TIMER_EVENT) {
		// add to timer event list in correct place (sorted by time mark)
		msg->Ref();
		VuTimerEvent* insert = (VuTimerEvent*)msg;
		VuTimerEvent* last = 0;
		VuTimerEvent* cur = timerlisthead_;
		while (cur && cur->mark_ <= insert->mark_) {
			last = cur;
			cur  = cur->next_;
		}
		insert->next_ = cur;
		
		if (last){
			last->next_    = insert;
		}
		else{
			timerlisthead_ = insert;
		}

		return TRUE;
	} 
	else{
		return VuMessageQueue::AddMessage(msg);
	}
}

////////////////////////
// VuPendingSendQueue //
////////////////////////
static VuResendMsgFilter resendMsgFilter;

VuPendingSendQueue::VuPendingSendQueue(int queueSize)
: VuMessageQueue(queueSize, &resendMsgFilter)
{
	bytesPending_ = 0;
}

VuPendingSendQueue::~VuPendingSendQueue(){
	DispatchMessages(-1, TRUE);
}

VU_BOOL VuPendingSendQueue::DispatchVuMessage(VU_BOOL autod){
	VuMessage* msg    = 0;
	VU_BOOL retval = FALSE;

	VuEnterCriticalSection();

	if (*read_) {
		msg = *read_;
		*read_++ = 0;
		if (read_ == tail_) {
			read_ = head_;
		}
		msg->Ref();
		bytesPending_ -= msg->Size();
		retval = TRUE;
		if (msg->DoSend()) {
			if (msg->Send() == 0) {
				retval = FALSE;
				if (!autod) {
					// note: this puts the unsent message on the end of the send queue
					AddMessage(msg);
				}
			}
		}
		msg->UnRef(); // list reference
		msg->UnRef(); // local reference
	}

	VuExitCriticalSection();
	return retval;
}

// static namespace
namespace {
	VU_BOOL TargetInvalidateCheck(VuMessage* msg, void *arg){
		if (msg->Target() == arg) {
			return TRUE;
		}
		return FALSE;
	}
} // namespace
 
void VuPendingSendQueue::RemoveTarget(VuTargetEntity* target){
	InvalidateQueueMessages(TargetInvalidateCheck, target);
}

VU_BOOL VuPendingSendQueue::AddMessage(VuMessage* msg){
	VU_BOOL retval = VuMessageQueue::AddMessage(msg);
	if (retval > 0) {
		bytesPending_ += msg->Size();
	}
	return retval;
}





