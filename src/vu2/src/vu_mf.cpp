/** @file vu_mf.cpp message filter implementation. */

#include "vu_mf.h"
#include "vuevent.h"


VU_BOOL VuResendMsgFilter::Test(VuMessage *message) const {
	if (
		(message->Flags() & VU_SEND_FAILED_MSG_FLAG) && 
		(message->Flags() & (VU_RELIABLE_MSG_FLAG | VU_KEEPALIVE_MSG_FLAG))
	){
		return TRUE;
	}
	else {
		return FALSE;
	}
}


#if 0

VuStandardMsgFilter::VuStandardMsgFilter() : msgTypeBitfield_(VU_VU_EVENT_BITS){}

VuStandardMsgFilter::VuStandardMsgFilter(ulong bitfield = VU_VU_EVENT_BITS) : msgTypeBitfield_(bitfield){}

VU_BOOL VuStandardMsgFilter::Test(VuMessage *message) const {
	ulong eventBit = 1<<message->Type();
	if ((eventBit & msgTypeBitfield_) == 0) {
		return FALSE;
	}
	if (eventBit & (VU_DELETE_EVENT_BITS | VU_CREATE_EVENT_BITS )) {
		return TRUE;
	}
	if (message->Sender() == vuLocalSession) {
		return FALSE;
	}
	// test to see if entity was found in database
	if (message->Entity() != 0) {
		return TRUE;
	}
	return FALSE;
}
	
VU_BOOL VuMessageTypeFilter::Test(VuMessage *event) const {
	return static_cast<VU_BOOL>((1<<event->Type() & msgTypeBitfield_) ? TRUE : FALSE);
}
#endif
