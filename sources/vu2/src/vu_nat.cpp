// Sfr: vu address part which needs commapi
#include "vutypes.h"
#include "InvalidBufferException.h"
#include "comms/capi.h"

///////////////
// VU_ADDRESS //
///////////////
bool VU_ADDRESS::IsPrivate() const{
	return (ComAPIPrivateIP(this->ip)) ? true : false;
}

void VU_ADDRESS::Decode(VU_BYTE **stream, long *rem){
	memcpychk(&recvPort, stream, sizeof(unsigned short), rem);
	memcpychk(&reliableRecvPort, stream, sizeof(unsigned short), rem);
	memcpychk(&ip, stream, sizeof(unsigned long), rem);
}

int VU_ADDRESS::Encode(VU_BYTE **stream){
	VU_BYTE *init = *stream;
	memcpy(*stream, &recvPort, sizeof(unsigned short));
	*stream += sizeof(unsigned short);
	memcpy(*stream, &reliableRecvPort, sizeof(unsigned short));
	*stream += sizeof(unsigned short);
	memcpy(*stream, &ip, sizeof(unsigned long));
	*stream += sizeof(unsigned long);
	// how much we wrote
	return *stream - init;
}

