#include "vu_filter.h"

//-----------------------------------------------------------------------------
// VuOpaqueFilter
//-----------------------------------------------------------------------------
VuOpaqueFilter::VuOpaqueFilter() : VuFilter(){
}

VuOpaqueFilter::VuOpaqueFilter(VuOpaqueFilter* other) : VuFilter(other){
}

VuOpaqueFilter::~VuOpaqueFilter(){
}

VU_BOOL VuOpaqueFilter::Test(VuEntity*){
	return FALSE;
}

int VuOpaqueFilter::Compare(VuEntity* ent1, VuEntity* ent2){
	return (int)ent1->Id() - (int)ent2->Id();
}

VuFilter *VuOpaqueFilter::Copy(){
	return new VuOpaqueFilter(this);
}

