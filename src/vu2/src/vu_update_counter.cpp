/** @file vu_update_counter.cpp
 * implementation of update counter stream functions
 * @author sfr
 */

#include "vutypes.h"
#include "InvalidBufferException.h"

int VU_UPDATE_COUNTER::Decode(VU_BYTE** buf, long *rem){
	std::memcpychk(&counter, buf, SerialSize(), rem);
	return SerialSize();
}

int VU_UPDATE_COUNTER::Encode(VU_BYTE **buf){
	memcpy(*buf, &counter, SerialSize());
	*buf += SerialSize();
	return SerialSize();
}
