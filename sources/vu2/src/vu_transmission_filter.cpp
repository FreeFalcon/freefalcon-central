/** @file vu_transmission_filter.cpp VuTransmissionFilter implementation. */

#include "vu2.h"

//-----------------------------------------------------------------------------
// VuTransmissionFilter
//-----------------------------------------------------------------------------

VuTransmissionFilter::VuTransmissionFilter() : VuKeyFilter(){
}

VuTransmissionFilter::VuTransmissionFilter(VuTransmissionFilter* other) : VuKeyFilter(other){
}

VuTransmissionFilter::~VuTransmissionFilter(){
}

VU_BOOL VuTransmissionFilter::Notice(VuMessage* event){
	if (event->Type() == VU_TRANSFER_EVENT){
		return TRUE;
	}
	else{
		return FALSE;
	}
}

VU_BOOL VuTransmissionFilter::Test(VuEntity* ent){
	return (VU_BOOL)(((ent->IsLocal() && (ent->UpdateRate() > (VU_TIME)0)) ? TRUE : FALSE));
}

VU_KEY VuTransmissionFilter::Key(const VuEntity* ent) const {
	return (ulong)(ent->LastTransmissionTime() + ent->UpdateRate());
}

int VuTransmissionFilter::Compare(VuEntity* ent1, VuEntity* ent2){
	ulong time1 = Key(ent1);
	ulong time2 = Key(ent2);

	return (time1 > time2 ? (int)(time1-time2) : -(int)(time2-time1));
}

VuFilter *VuTransmissionFilter::Copy(){
	return new VuTransmissionFilter(this);
}

