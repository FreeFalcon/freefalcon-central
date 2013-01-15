#ifndef _VUTYPES_H_
#define _VUTYPES_H_

// vutypes.h
// sfr: vu base types

#ifdef USE_SH_POOLS
#include "SmartHeap/Include/shmalloc.h"
#include "SmartHeap/Include/smrtheap.hpp"
#endif

typedef int VU_ERRCODE;
#define VU_ERROR	-1
#define VU_NO_OP	0
#define VU_SUCCESS	1

typedef void *VuMutex;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;

// note: BIG_SCALAR and SM_SCALAR are defined in vumath.h

typedef unsigned long VU_DAMAGE;
typedef unsigned long VU_TIME;

#define VU_TICS_PER_SECOND 1000

typedef unsigned char VU_BYTE;
typedef unsigned char VU_BOOL;
typedef signed char VU_TRI_STATE;	// TRUE, FALSE, or DONT_CARE

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
typedef	unsigned long VU_KEY;
typedef	unsigned long VU_ID_NUMBER;

// sfr: back to inline for efficiency
class VU_SESSION_ID {
public:
	// constructor
	VU_SESSION_ID() : value_(0) {}
	VU_SESSION_ID(unsigned long value) : value_((unsigned long)value) { }

	int operator == (const VU_SESSION_ID &rhs) const{
		return (value_ == rhs.value_ ? TRUE : FALSE); 
	}

	int operator != (const VU_SESSION_ID &rhs) const { 
		return (value_ != rhs.value_ ? TRUE : FALSE); 
	}

	int operator > (const VU_SESSION_ID &rhs) const { 
		return (value_ > rhs.value_ ? TRUE : FALSE); 
	}

	int operator >= (const VU_SESSION_ID &rhs) const { 
		return (value_ >= rhs.value_ ? TRUE : FALSE); 
	}

	int operator < (const VU_SESSION_ID &rhs) const { 
		return (value_ < rhs.value_ ? TRUE : FALSE); 
	}

	int operator <= (const VU_SESSION_ID &rhs) const { 
		return (value_ <= rhs.value_ ? TRUE : FALSE); 
	}

	operator unsigned long() const { 
		return (unsigned long) value_; 
	}

	// note: these are private to prevent (mis)use
private:
	int operator == (unsigned long &rhs) const ;
	int operator != (unsigned long &rhs) const ;
	int operator > (unsigned long &rhs) const ;
	int operator >= (unsigned long &rhs) const ;
	int operator < (unsigned long &rhs) const ;
	int operator <= (unsigned long &rhs) const ;

	// DATA
public:
	unsigned long 	value_;
};

class VU_ID {
public:
	//sfr: vu change
	VU_ID() : num_(0), creator_(0){}
	VU_ID(VU_SESSION_ID sessionpart, VU_ID_NUMBER idpart) : num_(idpart), creator_(sessionpart){}

	// basic operator overloading
	bool operator == (const VU_ID &rhs) const { 
		return (
			num_ == rhs.num_ ? 
			(creator_ == rhs.creator_ ? true : false) : 
			false
		); 
	}
	bool operator != (const VU_ID &rhs) const { 
		return (
			num_ == rhs.num_ ? 
			(creator_ == rhs.creator_ ? false : true) : 
			true
		); 
	}
	bool operator > (const VU_ID &rhs) const {
		if (creator_ > rhs.creator_){
			return true;
		}
		if (creator_ == rhs.creator_)	{
			if (num_ > rhs.num_){
				return true;
			}
		}
		return false;
	}
	bool operator >= (const VU_ID &rhs) const {
		if (creator_ > rhs.creator_){
			return true;
		}
		if (creator_ == rhs.creator_){
			if (num_ >= rhs.num_){
				return true;
			}
		}
		return false;
	}
	bool operator < (const VU_ID &rhs) const {
		if (creator_ < rhs.creator_){
			return true;
		}
		if (creator_ == rhs.creator_){
			if (num_ < rhs.num_){
				return true;
			}
		}
		return false;
	}
	bool operator <= (const VU_ID &rhs) const {
		if (creator_ < rhs.creator_){
			return true;
		}
		if (creator_ == rhs.creator_){
			if (num_ <= rhs.num_){
				return true;
			}
		}
		return false;
	}
	operator VU_KEY() const { 
		return (VU_KEY)(((unsigned short)creator_ << 16) | ((unsigned short)num_)); 
	}

	// note: these are private to prevent (mis)use
private:
	int operator == (const VU_KEY &rhs) const ;
	int operator != (VU_KEY &rhs) const ;
	int operator > (VU_KEY &rhs) const ;
	int operator >= (VU_KEY &rhs) const ;
	int operator < (VU_KEY &rhs) const ;
	int operator <= (VU_KEY &rhs) const ;

	// DATA
public:
	VU_ID_NUMBER		num_;
	VU_SESSION_ID 	creator_;
};

/** Represents an entity address. All entities are composed of 
* an IP address and 2 receive ports (one for reliable)
*/
class VU_ADDRESS {
public:
	/** default constructor
	* the receive ports always need to be specified
	*/
	VU_ADDRESS(
		unsigned long ip = 0,                                //< entity IP
		unsigned short recvPort = 0,//CAPI_UDP_PORT,         //< port where he receives
		unsigned short reliableRecvPort = 0 //CAPI_TCP_PORT  //< port where he receives reliable data
	){
		this->ip = ip;
		this->recvPort = recvPort;
		this->reliableRecvPort = reliableRecvPort;
	}

	// returns the struct size
	int Size() const {
		// ip + ports
		return sizeof(long) + sizeof(short)*2;
	}

	// equality: everything equal
	bool operator==(const VU_ADDRESS & rhs) const{
		return (
			(this->ip == rhs.ip) && 
			(this->recvPort == rhs.recvPort) &&
			(this->reliableRecvPort == rhs.reliableRecvPort)
		);
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
