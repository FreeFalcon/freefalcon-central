#ifndef VU_MF_H
#define VU_MF_H

/** @file vu_mf.h header file for message filters. */
#include "vutypes.h"

class VuMessage;

// filters used

/** base message filter. */
class VuMessageFilter {
public:
	VuMessageFilter() { }
	virtual ~VuMessageFilter() { }
	virtual VU_BOOL Test(VuMessage *event) const = 0;
	virtual VuMessageFilter *Copy() const = 0;
};

// provided default filters

/** allows only these events:
* - VuEvent(s) from a remote session
* - All Delete and Release events
* - All Create, FullUpdate, and Manage events
* it filters out these messages:
* - All update events on unknown entities
* - All update events on local entities
* - All non-event messages (though this can be overridden)
*/
class VuStandardMsgFilter : public VuMessageFilter {
public:
	VuStandardMsgFilter();
	VuStandardMsgFilter(ulong bitfield);
	virtual ~VuStandardMsgFilter(){}
	virtual VU_BOOL Test(VuMessage *event) const;
	virtual VuMessageFilter *Copy() const { return new VuStandardMsgFilter(msgTypeBitfield_); }

protected:
	ulong msgTypeBitfield_;
};

/** allows only messages for which send failed and are reliable or keep alive. */
class VuResendMsgFilter : public VuMessageFilter {
public:
	VuResendMsgFilter(){}
	virtual ~VuResendMsgFilter(){}
	virtual VU_BOOL Test(VuMessage *message) const;
	virtual VuMessageFilter *Copy() const { return new VuResendMsgFilter; }
};

/*
class VuMessageTypeFilter : public VuMessageFilter {
public:
	VuMessageTypeFilter(ulong bitfield) : msgTypeBitfield_(bitfield){}
	virtual ~VuMessageTypeFilter(){}
	virtual VU_BOOL Test(VuMessage *event) const;
	virtual VuMessageFilter *Copy() const { return new VuMessageTypeFilter(msgTypeBitfield_); }

protected:
	ulong msgTypeBitfield_;
};
*/

/** filter that lets everything goes through. */
class VuNullMessageFilter : public VuMessageFilter {
public:
	VuNullMessageFilter() : VuMessageFilter() { }
	virtual ~VuNullMessageFilter() { }
	virtual VU_BOOL Test(VuMessage *) const { return TRUE; }
	virtual VuMessageFilter *Copy() const { return new VuNullMessageFilter; }
};


#endif

