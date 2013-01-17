#include "vu2.h"
#include "vu_priv.h"

//-----------------------------------------------------------------------------
// VuFilter -- dummy base class
//-----------------------------------------------------------------------------

VuFilter::~VuFilter(){
}

VU_BOOL VuFilter::RemoveTest(VuEntity *){
	return TRUE;
}

VU_BOOL VuFilter::Notice(VuMessage *){
	return FALSE;
}
