#ifndef _VUTYPES_H_
#define _VUTYPES_H_
#include <iso646.h>

// vutypes.h
// sfr: vu base types

#ifdef USE_SH_POOLS
#include "smartheap/include/shmalloc.h"
#include "smartheap/include/smrtheap.hpp"
#endif

typedef int VU_ERRCODE;
#define VU_ERROR -1
#define VU_NO_OP 0
#define VU_SUCCESS 1

typedef void *VuMutex;
typedef unsigned int uint;
// NB: 'ulong' is a system typedef on Linux (<sys/types.h> == unsigned long) so it CANNOT be pinned
// to 32 bits here without a redefinition clash. On-disk campaign 'ulong' fields are 32-bit, so their
// reads/writes use memcpychk_u32 / a 32-bit member instead (see invalidbufferexception.h). #104.
typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;

// note: BIG_SCALAR and SM_SCALAR are defined in vumath.h

// #104: 32-bit, NOT `unsigned long`. These ride inside serialized structs (VuEntityType.updateRate_/damageSeed_
// in the .ct class table, plus VU network/save payloads) laid out by the original x86 build where long is 4 bytes.
// On Windows x64 (LLP64) long is still 4, so it worked; on LP64 Linux `unsigned long` is 8, which inflates every
// VuEntityType by 12 bytes and desyncs .ct deserialization (garbage dataPtr -> OOB write in LoadClassTable).
// `unsigned int` is 32-bit on every target we build, matching the on-disk/on-wire format exactly.
typedef unsigned int VU_DAMAGE;
typedef unsigned int VU_TIME;

#define VU_TICS_PER_SECOND 1000

typedef unsigned char VU_BYTE;
typedef unsigned char VU_BOOL;
typedef signed char VU_TRI_STATE; // TRUE, FALSE, or DONT_CARE

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef DONT_CARE
#define DONT_CARE -1
#endif

typedef unsigned char VU_MSG_TYPE;
typedef unsigned long VU_KEY;
// #104 (Linux LP64): VU_ID_NUMBER is a 32-bit id on disk and in the x86/Win64 (LLP64) reference ABI
// where 'unsigned long' is 4 bytes. On Linux LP64 it is 8 bytes, doubling sizeof(VU_ID) (num_ +
// creator_) from the on-disk 8 to 16 and desyncing every VU_ID read/write. Pin to 32 bits (no size
// change on Win32/Win64; Linux shrinks back to the reference width). Keep VU_SESSION_ID::value_ in sync.
typedef unsigned int VU_ID_NUMBER;

// sfr: back to inline for efficiency
class VU_SESSION_ID
{
public:
    // constructor
    VU_SESSION_ID() : value_(0)
    {
    }
    VU_SESSION_ID(unsigned long value) : value_((unsigned long)value)
    {
    }

    int operator==(const VU_SESSION_ID &rhs) const
    {
        return (value_ == rhs.value_ ? TRUE : FALSE);
    }

    int operator not_eq(const VU_SESSION_ID &rhs) const
    {
        return (value_ not_eq rhs.value_ ? TRUE : FALSE);
    }

    int operator>(const VU_SESSION_ID &rhs) const
    {
        return (value_ > rhs.value_ ? TRUE : FALSE);
    }

    int operator>=(const VU_SESSION_ID &rhs) const
    {
        return (value_ >= rhs.value_ ? TRUE : FALSE);
    }

    int operator<(const VU_SESSION_ID &rhs) const
    {
        return (value_ < rhs.value_ ? TRUE : FALSE);
    }

    int operator<=(const VU_SESSION_ID &rhs) const
    {
        return (value_ <= rhs.value_ ? TRUE : FALSE);
    }

    operator unsigned long() const
    {
        return (unsigned long)value_;
    }

    // note: these are private to prevent (mis)use
private:
    int operator==(unsigned long &rhs) const;
    int operator not_eq(unsigned long &rhs) const;
    int operator>(unsigned long &rhs) const;
    int operator>=(unsigned long &rhs) const;
    int operator<(unsigned long &rhs) const;
    int operator<=(unsigned long &rhs) const;

    // DATA
public:
    unsigned int
        value_; // #104 (LP64): 32-bit session id -- keeps sizeof(VU_ID)==8 (see VU_ID_NUMBER)
};

class VU_ID
{
public:
    //sfr: vu change
    VU_ID() : num_(0), creator_(0)
    {
    }
    VU_ID(VU_SESSION_ID sessionpart, VU_ID_NUMBER idpart)
        : num_(idpart), creator_(sessionpart)
    {
    }

    // basic operator overloading
    bool operator==(const VU_ID &rhs) const
    {
        return (num_ == rhs.num_ ? (creator_ == rhs.creator_ ? true : false) :
                                   false);
    }
    bool operator not_eq(const VU_ID &rhs) const
    {
        return (num_ == rhs.num_ ? (creator_ == rhs.creator_ ? false : true) :
                                   true);
    }
    bool operator>(const VU_ID &rhs) const
    {
        if (creator_ > rhs.creator_)
        {
            return true;
        }

        if (creator_ == rhs.creator_)
        {
            if (num_ > rhs.num_)
            {
                return true;
            }
        }

        return false;
    }
    bool operator>=(const VU_ID &rhs) const
    {
        if (creator_ > rhs.creator_)
        {
            return true;
        }

        if (creator_ == rhs.creator_)
        {
            if (num_ >= rhs.num_)
            {
                return true;
            }
        }

        return false;
    }
    bool operator<(const VU_ID &rhs) const
    {
        if (creator_ < rhs.creator_)
        {
            return true;
        }

        if (creator_ == rhs.creator_)
        {
            if (num_ < rhs.num_)
            {
                return true;
            }
        }

        return false;
    }
    bool operator<=(const VU_ID &rhs) const
    {
        if (creator_ < rhs.creator_)
        {
            return true;
        }

        if (creator_ == rhs.creator_)
        {
            if (num_ <= rhs.num_)
            {
                return true;
            }
        }

        return false;
    }
    operator VU_KEY() const
    {
        return (VU_KEY)(((unsigned short)creator_ << 16) bitor
                        ((unsigned short)num_));
    }

    // note: these are private to prevent (mis)use
private:
    int operator==(const VU_KEY &rhs) const;
    int operator not_eq(VU_KEY &rhs) const;
    int operator>(VU_KEY &rhs) const;
    int operator>=(VU_KEY &rhs) const;
    int operator<(VU_KEY &rhs) const;
    int operator<=(VU_KEY &rhs) const;

    // DATA
public:
    VU_ID_NUMBER num_;
    VU_SESSION_ID creator_;
};

/** Represents an entity address. All entities are composed of
* an IP address and 2 receive ports (one for reliable)
*/
class VU_ADDRESS
{
public:
    /** default constructor
    * the receive ports always need to be specified
    */
    VU_ADDRESS(unsigned long ip = 0, //< entity IP
               unsigned short recvPort =
                   0, //CAPI_UDP_PORT,         //< port where he receives
               unsigned short reliableRecvPort =
                   0 //CAPI_TCP_PORT  //< port where he receives reliable data
    )
    {
        this->ip = ip;
        this->recvPort = recvPort;
        this->reliableRecvPort = reliableRecvPort;
    }

    // returns the struct size
    int Size() const
    {
        // ip + ports  (#104: ip is 32-bit on the wire, not sizeof(long))
        return sizeof(int) + sizeof(short) * 2;
    }

    // equality: everything equal
    bool operator==(const VU_ADDRESS bitand rhs) const
    {
        return ((this->ip == rhs.ip) and (this->recvPort == rhs.recvPort) and
                (this->reliableRecvPort == rhs.reliableRecvPort));
    }


    // reads Vu address from stream
    void Decode(VU_BYTE **stream, long *rem);
    // writes Vu address to stream, returns ammount of written data
    int Encode(VU_BYTE **stream);
    // returns if an address comes from private network
    bool IsPrivate() const;

    // ports and ip data in host order
    unsigned short recvPort, reliableRecvPort;
    unsigned long ip;
};


#endif // _VUTYPES_H_
