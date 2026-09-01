// Sfr: vu address part which needs commapi
#include "vutypes.h"
#include "invalidbufferexception.h"
#include "comms/capi.h"

///////////////
// VU_ADDRESS //
///////////////
bool VU_ADDRESS::IsPrivate() const
{
    return (com_API_private_IP(this->ip)) ? true : false;
}

void VU_ADDRESS::Decode(VU_BYTE **stream, long *rem)
{
    memcpychk(&recvPort, stream, sizeof(unsigned short), rem);
    memcpychk(&reliableRecvPort, stream, sizeof(unsigned short), rem);
    memcpychk_u32(&ip, stream, rem); // #104: on-wire 32-bit ip
}

int VU_ADDRESS::Encode(VU_BYTE **stream)
{
    VU_BYTE *init = *stream;
    memcpy(*stream, &recvPort, sizeof(unsigned short));
    *stream += sizeof(unsigned short);
    memcpy(*stream, &reliableRecvPort, sizeof(unsigned short));
    *stream += sizeof(unsigned short);
    memcpy_u32(stream, &ip); // #104: on-wire 32-bit ip
    // how much we wrote
    return *stream - init;
}
