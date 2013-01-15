#if 0
// sfr: I inlined this stuff.
#include <string>
#include "InvalidBufferException.h"

using namespace std;

void std::memcpychk(void *dst, VU_BYTE **src, size_t size, long *rem){
	if ((size_t)*rem < size)	{
		char err[100];
		sprintf(err, "Trying to write %lld bytes to %ld buffer", size, *rem);
		string s = string(err);		throw InvalidBufferException(s);
	}
	memcpy(dst, *src, size);
	*rem -= size;
	*src += size;}
std::InvalidBufferException::InvalidBufferException(string msg): out_of_range(msg){

}


#endif
