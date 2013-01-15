#include "vu2.h"
#include "vu_priv.h"

//-----------------------------------------------------------------------------
// VuKeyFilter -- abstract class
//-----------------------------------------------------------------------------

VuKeyFilter::~VuKeyFilter(){
}

int VuKeyFilter::Compare (VuEntity* ent1, VuEntity* ent2){
	VU_KEY key1 = Key(ent1);
	VU_KEY key2 = Key(ent2);
	
	if (key1 > key2){
		return 1;
	}
	else if (key1 < key2){
		return -1;
	}
	return 0;
}

/*
ulong VuKeyFilter::Key(VuEntity* ent){
	return (ulong)ent->Id();
}*/
