#include "vu2.h"
#include "vu_priv.h"

//-----------------------------------------------------------------------------
// VuBiKeyFilter -- abstract class
//-----------------------------------------------------------------------------

VU_KEY VuBiKeyFilter::Key1(const VuEntity* ent) const {
#ifdef VU_GRID_TREE_Y_MAJOR
	return CoordToKey(ent->YPos());
#else
	return CoordToKey(ent->XPos());
#endif
}

VU_KEY VuBiKeyFilter::Key2(const VuEntity* ent) const {
#ifdef VU_GRID_TREE_Y_MAJOR
	return CoordToKey(ent->XPos());
#else
	return CoordToKey(ent->YPos());
#endif
}

VU_KEY VuBiKeyFilter::Key(const VuEntity* ent) const {
	return Key2(ent);
}

VU_KEY VuBiKeyFilter::CoordToKey(BIG_SCALAR coord) const { 
	int ret = FloatToInt32(factor_ * coord);
	if (ret < 0){ 
		ret = 0; 
	}
	else if (static_cast<unsigned int>(ret) >= res_){
		ret = res_ - 1;
	}
	return (VU_KEY)ret; 
}
