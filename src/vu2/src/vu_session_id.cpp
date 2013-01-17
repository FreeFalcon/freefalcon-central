/** Sfr: implementation of VU_SESSION_ID, declared in vutypes.h */

#include <stdlib.h>
#include <time.h>
#include "InvalidBufferException.h"


/*VU_SESSION_ID::VU_SESSION_ID(unsigned long ip, unsigned long value) {
	unsigned int seed = (unsigned int)clock();
	ip_ = ip;
	value_ = rand();
	ports_[0] = ports_[1] = 0; 
}

int VU_SESSION_ID::operator == (VU_SESSION_ID rhs) { 
	return (value_ == rhs.value_ ? TRUE : FALSE); 
}
*/

VU_SESSION_ID::VU_SESSION_ID() : value_(0) { 
}

VU_SESSION_ID::VU_SESSION_ID(unsigned long value): value_((unsigned long)value) { 
}

int VU_SESSION_ID::operator == (VU_SESSION_ID rhs){
	return (value_ == rhs.value_ ? TRUE : FALSE); 
}

int VU_SESSION_ID::operator != (VU_SESSION_ID rhs) { 
	return (value_ != rhs.value_ ? TRUE : FALSE); 
}

int VU_SESSION_ID::operator > (VU_SESSION_ID rhs) { 
	return (value_ > rhs.value_ ? TRUE : FALSE); 
}

int VU_SESSION_ID::operator >= (VU_SESSION_ID rhs) { 
	return (value_ >= rhs.value_ ? TRUE : FALSE); 
}

int VU_SESSION_ID::operator < (VU_SESSION_ID rhs) { 
	return (value_ < rhs.value_ ? TRUE : FALSE); 
}

int VU_SESSION_ID::operator <= (VU_SESSION_ID rhs) { 
	return (value_ <= rhs.value_ ? TRUE : FALSE); 
}

VU_SESSION_ID::operator unsigned long() { 
	return (unsigned long) value_; 
}

/*
void VU_SESSION_ID::SetIP(unsigned long  ip){
	ip_ = ip;
}

void VU_SESSION_ID::SetValue(unsigned long value){
	value_ = value;
}*/



