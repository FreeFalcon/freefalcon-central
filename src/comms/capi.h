// CONFIGURATION notes:
// within: capiopt.h #define LOAD_DLLS for expicit LoadLibrary() calls to be
// used for WS2_32.DLL and DPLAYX.DLL and OLE32.DLL. If not #defined 
// application must link with WS2_32.LIB and DPLAYX.LIB and OLE32.LIB otherwise
// these LIBs are not needed.


#pragma once


#ifdef __cplusplus
extern "C" {
#endif


#define CAPI_VERSION 1
#define CAPI_REVISION 4
#define CAPI_PATCH 0
#define CAPI_REVISION_DATE "5/27/98"
#define CAPI_PATCH_DATE "5/27/98"

// Protocol values for com_API_enum_protocols.
#define CAPI_UNKNOWN_PROTOCOL 0
#define CAPI_UDP_PROTOCOL 1
#define CAPI_IP_MULTICAST_PROTOCOL 2
#define CAPI_SERIAL_PROTOCOL 3
#define CAPI_TEN_PROTOCOL 4
#define CAPI_TCP_PROTOCOL 5
#define CAPI_DPLAY_MODEM_PROTOCOL 6
#define CAPI_DPLAY_SERIAL_PROTOCOL 7
#define CAPI_DPLAY_TCP_PROTOCOL 8
#define CAPI_DPLAY_IPX_PROTOCOL 9
#define CAPI_RUDP_PROTOCOL 10
#define CAPI_GROUP_PROTOCOL 11
#define CAPI_LAST_PROTOCOL CAPI_GROUP_PROTOCOL

#define CAPI_HOST 1
#define CAPI_JOIN 0

// COMAPI Error codes to return to application.
#define COMAPI_BAD_HEADER -1 // COMAPI message header not correct.
#define COMAPI_OUT_OF_SYNC -2 // Data not quite as we expected.
#define COMAPI_OVERRUN_ERROR -3 // Trying to read too much.
#define COMAPI_BAD_MESSAGE_SIZE -4 // Internal syncing error.
#define COMAPI_CONNECTION_CLOSED -5 // Remote connection closed gracefully.
#define COMAPI_MESSAGE_TOO_BIG -6
#define COMAPI_CONNECTION_PENDING -7
#define COMAPI_WOULDBLOCK -8
#define COMAPI_EMPTYGROUP -9
#define COMAPI_PLAYER_LEFT -10
#define COMAPI_NOTAGROUP -11
#define COMAPI_CONNECTION_TIMEOUT -12
#define COMAPI_HOSTID_ERROR -13
#define COMAPI_WINSOCKDLL_ERROR -14 // WS2_32.DLL
#define COMAPI_DPLAYDLL_ERROR -15 // DPLAYX.DLL
#define COMAPI_OLE32DLL_ERROR -16 // OLE32.DLL for DirectPLay
#define COMAPI_TENDLL_ERROR -17

// ComAPIQuery() query types.
#define COMAPI_MESSAGECOUNT 1
#define COMAPI_RECV_WOULDBLOCKCOUNT 2
#define COMAPI_SEND_WOULDBLOCKCOUNT 3
#define COMAPI_CONNECTION_ADDRESS 4
#define COMAPI_RECEIVE_SOCKET 5
#define COMAPI_SEND_SOCKET 6
#define COMAPI_RELIABLE 7
#define COMAPI_RECV_MESSAGECOUNT 8
#define COMAPI_SEND_MESSAGECOUNT 9
#define COMAPI_UDP_CACHE_SIZE 10
#define COMAPI_RUDP_CACHE_SIZE 12
#define COMAPI_MAX_BUFFER_SIZE 13 // 0 = no MAXIMUM = stream
#define COMAPI_ACTUAL_BUFFER_SIZE 14
#define COMAPI_PROTOCOL 15
#define COMAPI_STATE 16
#define COMAPI_DPLAY_PLAYERID 17
#define COMAPI_DPLAY_REMOTEPLAYERID 18
#define COMAPI_SENDER 19
#define COMAPI_TCP_HEADER_OVERHEAD 20
#define COMAPI_UDP_HEADER_OVERHEAD 21
#define COMAPI_RUDP_HEADER_OVERHEAD 22
#define COMAPI_PING_TIME 23
#define COMAPI_BYTES_PENDING 24
#define COMAPI_SENDER_PORT 25 // Converts port information.
#define COMAPI_ID 26 // ID of sender.

#define COMAPI_STATE_CONNECTION_PENDING 0
#define COMAPI_STATE_CONNECTED 1
#define COMAPI_STATE_ACCEPTED 2

#define CAPI_THREAD_PRIORITY_ABOVE_NORMAL 1
#define CAPI_THREAD_PRIORITY_BELOW_NORMAL 2
#define CAPI_THREAD_PRIORITY_HIGHEST 3
#define CAPI_THREAD_PRIORITY_IDLE 4
#define CAPI_THREAD_PRIORITY_LOWEST 5
#define CAPI_THREAD_PRIORITY_NORMAL 6
#define CAPI_THREAD_PRIORITY_TIME_CRITICAL 7

#define CAPI_DPLAY_NOT_GUARANTEED 0
#define CAPI_DPLAY_GUARANTEED 1

// Define for default IP used for dangling comms.
#define CAPI_DANGLING_IP 0xFFFFFFFF
#define CAPI_DANGLING_ID 0xFFFFFFFF

// BW states, also defined in comAPIbwcontrol BWstate enum.
#define CAPI_LOBBY_ST 0
#define CAPI_CAS_ST 1
#define CAPI_CAC_ST 2
#define CAPI_DF_ST 3

// Message types, for bandwidth usage, also defined in comAPIbwcontrol.
// Important CAPI_JOIN_BWTYPE must be >= BW_NUM_TYPES in capibwcontrol.
#define CAPI_POSUPD_BWTYPE 0
#define CAPI_DIRTY_BWTYPE 1
#define CAPI_OTHER_BWTYPE 2
#define CAPI_COMMON_BWTYPE 3
#define CAPI_JOIN_BW_TYPE 4

typedef struct ComApiHandle* com_API_handle;
	// Initialize comms optional - done automatically by any open, but needed
	// if calling utility functions before opening a handle.
	int com_API_initialize_communications(void);
	void com_API_set_name(com_API_handle, char*);
	// All these functions receive and return in host order.
	// Sets comm ports (best effort and reliable). 
	void com_API_set_local_ports(unsigned short b, unsigned short r);
	unsigned long com_API_get_peer_IP(com_API_handle c);
	unsigned short com_API_get_receive_port(com_API_handle c);
	unsigned short com_API_get_peer_receive_port(com_API_handle c);
	int com_API_get_protocol(com_API_handle c);
	unsigned long com_API_get_peer_ID(com_API_handle c);
	// The globals are defined in CAPI.c
	unsigned short com_API_get_my_receive_port();
	unsigned short com_API_get_my_reliable_receive_port();
	void com_API_set_my_receive_port(unsigned short);
	void com_API_set_my_reliable_receive_port(unsigned short);
	// Checks if an IP comes from a private network
	// IP is in host order
	int com_API_private_IP(unsigned long ip);

	// Enumerate available protocols.
	int com_API_enum_protocols(int* protocols, int max_protocols);

	com_API_handle com_IP_multicast_open(int buffer_size, char* game_name,
									  int mc_scope);


	// TCP specific open.
	// Begin a TCP connection as a listener to wait for connections.
	com_API_handle com_TCP_open_listen(int buffer_size, char* game_name, 
									int TCP_port, 
									void(*accept_callback)
									(com_API_handle c, int ret));

	// Begin a TCP connection to a targeted listening TCP listener.
	com_API_handle com_TCP_open_connect(int buffer_size, char* game_name, 
										int TCP_port, unsigned long IP_address,
										void(*connect_callback)
										(com_API_handle c, int ret),
										int timeout_seconds);

	// Get handle for an anticipated acception from target IP.
	// Fails if no listening handle has not been previously opened on the port.
	com_API_handle com_TCP_open_accept(unsigned long IP_address, int TCP_port,
									   int timeout_seconds);

	// Get a handle to use with ComAPISend for sending data to all TCP
	// connections. Will send to all open connections, either accept() or
	// connect() type will ignore listen sockets.
	com_API_handle com_TCP_get_group_handle(int buffer_size);

	//com_API_handle ComDPLAYOpen(int protocol, int mode, char* address,
	//                            int buffer_size, void* guid, 
	//                            void (*connect_callback)
	//                            (com_API_handle c, int ret),
	//                            int timeout_seconds);

	//void ComAPIDPLAYSendMode(com_API_handle c, int sendmode);   /* default is GUARANTEED */


	/* end comms session */
	void ComAPIClose(com_API_handle c);

	// send and receive data from comms
	int ComAPISendOOB(com_API_handle c, int msgsize, int type);
	int ComAPISend(com_API_handle c, int msgsize, int type);
	int ComAPISendDummy(com_API_handle c, unsigned long ip, unsigned short port);

	int ComAPIGet(com_API_handle c);

	void ComAPIRegisterInfoCallback(void(*func)(com_API_handle c, int send, int msgsize));

	//////////////////
	// BW FUNCTIONS //
	//////////////////
	// sfr: changed bw functions
	/** starts bw control */
	void ComAPIBWStart();
	/** gets local bandwidth, bytes per second */
	int ComAPIBWGet(void);
	/** called when a player joins, adjusting bw */
	void ComAPIBWPlayerJoined();
	/** called when a player leaves, adjusting bw */
	void ComAPIBWPlayerLeft();
	/** enters a given state: CAPI_LOBBY_ST, CAPI_CAS_ST, CAC_ST e DF_ST */
	void ComAPIBWEnterState(int state);
	/** gets BW situation for a connection: 0 ok, 1 yellow, 2 or more critical */
	int ComAPIBWGetStatus(int isReliable);


	/* set the group to send and recieve data from */
	void ComAPIGroupSet(com_API_handle c, int group);

	/* get the local hosts unique id */
	int ComAPIHostIDLen(com_API_handle c);
	int ComAPIHostIDGet(com_API_handle c, char *buf, int reset);

	/* get the associated buffers */
	char *ComAPISendBufferGet(com_API_handle c);
	char *ComAPIRecvBufferGet(com_API_handle c);

	/* query connection information - not all options are supported on all protocols */
	/* refer to query types #defined above */
	unsigned long ComAPIQuery(com_API_handle c, int querytype);

	/* Group functions*/
	com_API_handle ComAPICreateGroup(char *name, int messagesize, ...);
	/* Must call with 0 terminating parameter ie: ComCreateGroup(1024,0)
	If called with comhandles , these will be used to determine Group Message size
	ie:   ComCreateGroup(1024,,ch1,ch2,0)  */


	/* member may be a group handle or a connection handle */
	int ComAPIAddToGroup(com_API_handle grouphandle, com_API_handle memberhandle);
	int ComAPIDeleteFromGroup(com_API_handle grouphandle, com_API_handle memberhandle);
	com_API_handle CAPIIsInGroup(com_API_handle grouphandle, unsigned long ipAddress);
	/* Send to a group but exclude Xhandle */
	//int ComAPISendX(com_API_handle group, int msgsize, com_API_handle Xhandle );

	/* Close all Open IP handles */
	void ComAPICloseOpenHandles(void);

	/* Convert host long IP address to string */
	char *ComAPIinet_htoa(unsigned long ip);

	/* Convert net long IP address to string */
	char *ComAPIinet_ntoa(unsigned long ip);
	/* Convert dotted string ipa ddress to host long */
	unsigned long ComAPIinet_haddr(char * IPaddress);

	/* reports last error for ComOPen calls */
	unsigned long ComAPIGetLastError(void);

	unsigned long ComAPIGetNetHostBySocket(int Socket);
	unsigned long ComAPIGetNetHostByHandle(com_API_handle c);

	/* TIMESTAMP functions */
	/* OPTIONAL call to set the timestamp function */
	void ComAPISetTimeStampFunction(unsigned long(*TimeStamp)(void));

	/* get the timestamp associated with the most recent ComAPIGet()
	returns 0 if no timestamp function has been defined */
	unsigned long ComAPIGetTimeStamp(com_API_handle c);

	/****  Protoype for TimeStamp callback
	unsigned long ComAPIGetTimeStamp(com_API_handle c)
	***/


	/* sets the rececive thread priority for UDP .. receive thread .. if exists*/
	void ComAPISetReceiveThreadPriority(com_API_handle c, int priority);


	/** sfr: translates address to ip. Answer in machine order */
	long ComAPIGetIP(const char *address);


#ifdef __cplusplus
}
#endif