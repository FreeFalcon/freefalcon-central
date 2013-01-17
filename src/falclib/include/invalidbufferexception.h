#ifndef _INVALID_BUFFER_EXCEPTION_
#define _INVALID_BUFFER_EXCEPTION_

#include <stdexcept>
#include <string.h>
#include "vutypes.h" //for VU_BYTE

//sfr: this is just for test purposes
//#define MP_DEBUG

/** sfr: this class is used for multiplayer receiving functions, mainly those based on streams.
* When we read a bad buffer, we should throw InvalidBufferException, which extends out_of_range
* exception
*/
//namespace std {

class InvalidBufferException: public std::out_of_range {
public:
	InvalidBufferException(const std::string &msg) : out_of_range(msg){}
};

/** this function is a memcpy just a little modified, the difference is: 
* it throws InvalidBufferException if rem < size
*/
inline void memcpychk(void *dst, VU_BYTE **src, size_t size, long *rem){
	if ((size_t)*rem < size){
		char err[100];
		sprintf(err, "Trying to write %lu bytes to %ld buffer", static_cast<unsigned long>(size), *rem);
		std::string s(err);
		throw InvalidBufferException(s);
	}
	memcpy(dst, *src, size);
	*rem -= size;
	*src += size;
}
//}

#endif

