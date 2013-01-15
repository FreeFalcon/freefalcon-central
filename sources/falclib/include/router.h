//
// Router.h
// 
// Router layer for comms shit
//
// Kevin Klemmick
// Sept, 1997
// ====================================================================================

#ifndef ROUTER_H
#define ROUTER_H

// =========================================================
// These are the ways we can communicate with other machines
// =========================================================

// Protocols available to Router
#define RTR_UDP_AVAILABLE		0x01			// We have UDP available
#define RTR_TCP_AVAILABLE		0x02			// We have TCP available

// Virtual connection types available to Router
#define RTR_PTOP_AVAILABLE		0x01			// We can send point to point messages
#define RTR_BCAST_AVAILABLE		0x02			// We can send broadcast (to world) messages
#define RTR_SERVER_AVAILABLE	0x04			// We are connecting to an exploder server

// Virtual connection type equivalencies
#define RTR_LAN					0x03			// PtoP and Bcast are available
#define RTR_INET				0x01			// PtoP only
#define RTR_INET_SERVER			0x04			// Server only
#define RTR_NULL_MODEM			0x01			// PtoP only
#define RTR_MODEM_MODEM			0x01			// PtoP only
#define RTR_MODEM_INET			0x01			// PtoP only
#define RTR_MODEM_SERVER		0x04			// Server only

#define RTR_NAME_SIZE			8				// Size, in bytes, of group's unique ids

#define RTR_RELIABLE			1
#define RTR_NORMAL				0

#define CAPI_UDP_PORT			2934			// Note: This unfortunately needs to match the value in comms\mcast.h
#define CAPI_TCP_PORT			2935

// =========================================================
// Return values
// =========================================================

#define RTR_OK							1
#define RTR_ERROR       				0

#define RTR_INVALID_HANDLE				-1
#define RTR_PROTOCOL_NOT_AVAILABLE		-2
#define RTR_OVERFLOW_ERROR		        -3		// Send buffer is full
#define RTR_CONNECTION_CLOSED			-5		// Connection is no longer available
#define RTR_MESSAGE_TOO_BIG				-6 
#define RTR_CONNECTION_PENDING			-7
#define RTR_WOULDBLOCK					-8
#define RTR_EMPTYGROUP					-9
#define RTR_NOTHING_TO_SEND				-10
#define RTR_WRONG_ADDRESS				-11
#define RTR_NOTHING_READ                -12

// =========================================================
// Router types and classes
// =========================================================

typedef struct comapihandle *ComAPIHandle;      // defined in CAPI.h

typedef unsigned long	RtrAddress;				// IP Address or similar way to get to a physical machine
typedef unsigned char	uchar;

class RouterAddressNode {
	public:
		RtrAddress			machine_address;	// Physical address of the machine
		ComAPIHandle		normalHandle;		// UDP or unreliable transport handle (if any)
		ComAPIHandle		reliableHandle;		// TCP or reliable transport handle (if any)
		RouterAddressNode	*next;				// Next node in the delivery list
	public:
		RouterAddressNode (RtrAddress pa, ComAPIHandle remote_socket);
		~RouterAddressNode (void);

		RtrAddress GetAddress (void)		{ return machine_address; };
	};

typedef RouterAddressNode* RtrAddressNode;

class RouterHandle {
	public:
		RtrAddressNode		delivery_list;		// List of physical addresses to deliver to
		uchar				name[RTR_NAME_SIZE];// The handle's name
		RouterHandle		*next;				// Next handle in list
	public:
		RouterHandle (void);
		~RouterHandle (void);

		void AddToDeliveryList (RtrAddress pa);
		void RemoveFromDeliveryList (RtrAddress pa);
	};

typedef RouterHandle* RtrHandle;

class RouterInfoClass {
	public:
		uchar				type;				// type of virtual connection we have
		uchar				protocols;			// protocols available to us
		RtrAddressNode		server_info;		// server address && connections, if one exists
		RtrHandle			server_handle;
		RtrAddressNode		broadcast_info;		// broadcast information, if any exists
		RtrHandle			broadcast_handle;
		RtrHandle			group_list;			// list of virtual groups we know about
		RtrAddressNode		connection_list;	// list of all current connections
		RtrAddress			our_address;		// who we are
		short				max_message_size;	// Biggest message we're allowed to send
		char*				game_name;			// Name of the game - duh!
	public:
		RouterInfoClass(void);
	};

// =========================================================
// Here are our functions
// =========================================================

extern RtrHandle RtrInitRouter (int virtual_connection_type, 
								int protocols_available, 
								RtrAddress our_address, 
								RtrAddress initial_address, 
								int max_message_size,
								char* game_name);

extern void RtrShutdownRouter (void);

extern RtrHandle RtrCreateGroup (uchar* name);

extern void RtrDeleteGroup (RtrHandle group);

extern int RtrAddToGroup (RtrHandle group, RtrHandle connection);

extern int RtrRemoveFromGroup (RtrHandle group, RtrHandle connection);

extern RtrHandle RtrCreateConnection (RtrAddress physical_address, uchar* name);

extern void RtrShutdownConnection (RtrHandle connection);

// RtrGetSendBuffer requires to buffer to be filled by the caller.
// It is guarenteed not to send until another call to RtrGetSendBuffer or RtrSendNow.
// Multiple calls to RtrGetSendBuffer can be made without calling RtrSendNow.
extern int RtrGetSendBuffer (RtrHandle to, uchar* bufptr, short size, int reliable);

// RtrSendNow sends the currently pending Rtr buffer.
extern int RtrSendNow (void);

// RtrGet will get the next message addressed to this machine
extern int RtrGet (uchar* bufptr);

// RtrGetLocalId will return the IPAddress of the local machine, or some other unique id
extern RtrAddress RtrGetLocalId (void);

#endif