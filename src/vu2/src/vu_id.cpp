/** sfr: VU_ID implementation
* I took this out of apitypes header as any change in it resulted in almost everything
* being compiled again.
*/


#include "vutypes.h" // where declaration is


VU_ID::VU_ID() : creator_(0), num_(0) {

}

VU_ID::VU_ID(VU_SESSION_ID sessionpart, VU_ID_NUMBER idpart): creator_(sessionpart), num_(idpart) {

}

// sfr: vu change
int VU_ID::operator == (VU_ID rhs){ 
	return ((num_ == rhs.num_) && (creator_ == rhs.creator_)) ? TRUE : FALSE;
}

int VU_ID::operator != (VU_ID rhs) { 
	return ((num_ == rhs.num_) && (creator_ == rhs.creator_)) ? FALSE : TRUE;
}

int VU_ID::operator > (VU_ID rhs) {
	if (
		(creator_ > rhs.creator_) ||
		((creator_ == rhs.creator_) && (num_ > rhs.num_))
	){
		return TRUE;
	}

	return FALSE;
}

int VU_ID::operator >= (VU_ID rhs) {
	if (
		(creator_ > rhs.creator_) ||
		((creator_ == rhs.creator_) && (num_ >= rhs.num_))
	){
		return TRUE;
	}
	return FALSE;
}

int VU_ID::operator < (VU_ID rhs) {
	if (
		(creator_ < rhs.creator_) ||
		((creator_ == rhs.creator_) && (num_ < rhs.num_))
	){
		return TRUE;
	}
	return FALSE;
}

int VU_ID::operator <= (VU_ID rhs) {
	if (
		(creator_ < rhs.creator_) ||
		((creator_ == rhs.creator_) && (num_ <= rhs.num_))
	){
		return TRUE;
	}
	return FALSE;
}

VU_ID::operator VU_KEY(){ 
	return (VU_KEY)
		(((unsigned short)creator_ << 16) | ((unsigned short)num_)); 
}

