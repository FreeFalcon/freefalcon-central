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

class InvalidBufferException : public std::out_of_range
{
public:
    InvalidBufferException(const std::string &msg) : out_of_range(msg)
    {
    }
};

/** this function is a memcpy just a little modified, the difference is:
* it throws InvalidBufferException if rem < size
*/
inline void memcpychk(void *dst, VU_BYTE **src, size_t size, long *rem)
{
    if ((size_t)*rem < size)
    {
        char err[100];
        sprintf(err, "Trying to write %lu bytes to %ld buffer",
                static_cast<unsigned long>(size), *rem);
        std::string s(err);
        throw InvalidBufferException(s);
    }

    memcpy(dst, *src, size);
    *rem -= size;
    *src += size;
}

// #104 (Linux LP64): campaign/network streams store 'long' fields as 32 bits (x86 / Win64 LLP64,
// where long is 4 bytes). On Linux LP64 'long' is 8 bytes, so a raw memcpychk of sizeof(long)
// over-reads 4 bytes per field and desyncs the stream. Read each on-disk 32-bit value and widen
// into the native long field(s). On x86/Win64 (long == 4) these collapse to a plain memcpychk.
inline void memcpychk_l32(void *dst, VU_BYTE **src, size_t nElems, long *rem)
{
    if (sizeof(long) == 4)
    {
        memcpychk(dst, src, sizeof(long) * nElems, rem);
        return;
    }
    long *d = (long *)dst;
    for (size_t i = 0; i < nElems; ++i)
    {
        int v = 0;
        memcpychk(&v, src, 4, rem);
        d[i] = v;
    }
}
// Unsigned counterpart for on-disk 32-bit 'ulong'/flag fields: zero-extend each 4-byte disk value into
// the (possibly 64-bit) native field(s). Templated so it accepts unsigned long / unsigned int dests.
// nElems>1 handles 'ulong arr[N]' bitmask arrays (zero-extend, never sign-extend -- bit31 stays clean).
template <class T>
inline void memcpychk_u32(T *dst, VU_BYTE **src, long *rem, size_t nElems = 1)
{
    if (sizeof(T) == 4)
    {
        memcpychk(dst, src, 4 * nElems, rem);
        return;
    }
    for (size_t i = 0; i < nElems; ++i)
    {
        unsigned int v = 0;
        memcpychk(&v, src, 4, rem);
        dst[i] = v;
    }
}
// Encode counterpart: narrow native long field(s) to 32 bits into a raw (bounds-unchecked) buffer.
inline void memcpy_l32(VU_BYTE **dst, const void *src, size_t nElems)
{
    if (sizeof(long) == 4)
    {
        memcpy(*dst, src, sizeof(long) * nElems);
        *dst += sizeof(long) * nElems;
        return;
    }
    const long *s = (const long *)src;
    for (size_t i = 0; i < nElems; ++i)
    {
        int v = (int)s[i];
        memcpy(*dst, &v, 4);
        *dst += 4;
    }
}
// Encode counterpart of memcpychk_u32: narrow a native (possibly 64-bit) unsigned field to 32 bits
// into a raw (bounds-unchecked) buffer, advancing the write pointer. Templated so it accepts
// unsigned long / unsigned int / DWORD sources. On x86/Win64 this is a plain 4-byte memcpy.
template <class T>
inline void memcpy_u32(VU_BYTE **dst, const T *src, size_t nElems = 1)
{
    for (size_t i = 0; i < nElems; ++i)
    {
        unsigned int v = (unsigned int)src[i];
        memcpy(*dst, &v, 4);
        *dst += 4;
    }
}
// On-disk width of a serialized 'long' (4 on every target ABI). Use in SaveSize()-style accounting.
static const size_t DISK_LONG = 4;
//}

#endif
