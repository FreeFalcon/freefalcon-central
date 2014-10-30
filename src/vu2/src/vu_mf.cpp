/** @file vu_mf.cpp message filter implementation. */

#include "vu_mf.h"
#include "vuevent.h"


VU_BOOL VuResendMsgFilter::Test(VuMessage *message) const
{
    if (
        (message->Flags() bitand VU_SEND_FAILED_MSG_FLAG) and 
        (message->Flags() bitand (VU_RELIABLE_MSG_FLAG bitor VU_KEEPALIVE_MSG_FLAG))
    )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


#if 0

VuStandardMsgFilter::VuStandardMsgFilter() : msgTypeBitfield_(VU_VU_EVENT_BITS) {}

VuStandardMsgFilter::VuStandardMsgFilter(ulong bitfield = VU_VU_EVENT_BITS) : msgTypeBitfield_(bitfield) {}

VU_BOOL VuStandardMsgFilter::Test(VuMessage *message) const
{
    ulong eventBit = 1 << message->Type();

    if ((eventBit bitand msgTypeBitfield_) == 0)
    {
        return FALSE;
    }

    if (eventBit bitand (VU_DELETE_EVENT_BITS bitor VU_CREATE_EVENT_BITS))
    {
        return TRUE;
    }

    if (message->Sender() == vuLocalSession)
    {
        return FALSE;
    }

    // test to see if entity was found in database
    if (message->Entity() not_eq 0)
    {
        return TRUE;
    }

    return FALSE;
}

VU_BOOL VuMessageTypeFilter::Test(VuMessage *event) const
{
    return static_cast<VU_BOOL>((1 << event->Type() bitand msgTypeBitfield_) ? TRUE : FALSE);
}
#endif
