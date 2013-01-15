#ifndef _CAPIPRIV_H_
#define _CAPIPRIV_H_

	
#if WIN32
#include <winsock.h>
#endif
#include "capi.h"

#ifdef __cplusplus
extern "C" {
#endif

#if WIN32
typedef struct _WSABUF {
	u_long      len;     /* the length of the buffer */
	char FAR *  buf;     /* the pointer to the buffer */
} WSABUF, FAR * LPWSABUF;
#else 
typedef struct _WSABUF {
	  u_long      len;      /* the length of the buffer */
	  char        *buf;     /* the pointer to the buffer */
} WSABUF, *LPWSABUF;
#endif


/***  define LOAD_DLLS for expicit LoadLibrary() calls will be used for WS2_32.DLL and DPLAYX.DLL ***/
/***  If not defined  application must link with WS2_32.LIB and DPLAYX.LIB                        ***/
/* #define LOAD_DLLS       */
/***********************************************************************************************/


//#define GAME_NAME_LENGTH 16
#define GAME_NAME_LENGTH 4

/* extern GLOBAL WSA startup reference count, defined in  WS2Init() */
#if WIN32
extern int WS2Connections;
extern HINSTANCE  hWinSockDLL;
#endif

typedef			 unsigned long	(*DWProc_t)();

// sfr: added ID information
typedef struct comapiheader {
	char gamename[GAME_NAME_LENGTH];
	unsigned long id; ///< this is the ID of the sender
} ComAPIHeader;

#define MAX_RUDP_HEADER_SIZE	(sizeof(ComAPIHeader)+9)		// Maximum amount of crap we'll take on our packets

typedef struct capilist {
	struct capilist      *next;
	char				 *name;
	ComAPIHandle          com;
#ifdef CAPI_NET_DEBUG_FEATURES
	void                 *data;
	int                   size;
	int                   sendtime;
#endif
} CAPIList;

typedef struct comapihandle {
	char *name;   // name
	int protocol; // protocol
	// send and receive functions for this comm
	int (*send_func)(struct comapihandle *c, int msgsize, int oob, int type);
	int (*send_dummy_func)(struct comapihandle *c, unsigned long ip, unsigned short port);
	int (*sendX_func)(struct comapihandle *c, int msgsize, int oob, int type, struct comapihandle *Xcom );
	int (*recv_func)(struct comapihandle *c);
	// buffer functions
	char * (*send_buf_func)(struct comapihandle *c);
	char * (*recv_buf_func)(struct comapihandle *c);
	// address function
	int (*addr_func)(struct comapihandle *c, char *buf, int reset);
	// close function
	void (*close_func)(struct comapihandle *c);
	// query function
	unsigned long (*query_func)(struct comapihandle *c, int querytype);
	// timestamp function
	unsigned long (*get_timestamp_func)(struct comapihandle *c);
} ComAPI;

typedef struct reliable_packet
{
	unsigned long last_sent_at;				/* time this packet was last sent */
	unsigned short sequence_number;
	unsigned short message_number;
	unsigned short size;
	unsigned char send_count;
	unsigned char oob;
	unsigned char message_slot;
	unsigned char message_parts;
	unsigned char dispatched;				/* has the application seen us yet? */
	unsigned char acknowledged;				/* have we sent the ack out of sequence */
	
	char *data;
	
	struct reliable_packet *next;
} Reliable_Packet;


typedef struct reliable_data
{
	unsigned short sequence_number;		/* sequence number for sending */
	unsigned short oob_sequence_number;	/* sequence number for sending */

	short message_number;				/* message number for sending */
	Reliable_Packet *sending;			/* list of sent packets */
	Reliable_Packet *last_sent;			/* last of the sending packets */
	
	int reset_send;						/* What is the reset stage we are in */
	
	int last_sequence;					/* other's last seen sequence number */
	int last_received;					/* my last received sequential sequence number for ack.*/
	int last_sent_received;				/* the last last_received that I acknowledged */
	int send_ack;						/* we need to send an ack packet */

	int last_dispatched;				/* the last packet that I dispatched */
	Reliable_Packet *receiving;			/* list of received packets */

	int sent_received;					/* what the last received I sent */
	unsigned long last_send_time;		/* last time we checked for ack */

	Reliable_Packet *oob_sending;		/* list of sent packets */
	Reliable_Packet *oob_last_sent;			/* last of the sending packets */

	int last_oob_sequence;				/* other's last seen sequence number */
	int last_oob_received;				/* my last received sequential sequence number for ack.*/
	int last_oob_sent_received;			/* the last last_received that I acknowledged */
	int send_oob_ack;					/* we need to send an ack packet */

	int last_oob_dispatched;			/* the last packet that I dispatched */
	Reliable_Packet *oob_receiving;		/* list of received packets */
	
	int sent_oob_received;				/* what the last received I sent */
	unsigned long last_oob_send_time;	/* last time we checked for ack */
	
	long last_ping_send_time;			/* when was the last time I sent a ping */
	long last_ping_recv_time;			/* the time we received on a ping */
	char *real_send_buffer;				/* buffer actually sent down the wire (after encoding) */
} Reliable_Data;


typedef struct comiphandle {
	struct comapihandle apiheader;

	int buffer_size;
	int max_buffer_size;
	int ideal_packet_size;
	WSABUF send_buffer;
	WSABUF recv_buffer;
	WSADATA wsaData;
	char * compression_buffer;

	//sfr: 
	struct sockaddr_in sendAddress; // we send data to this address
	struct sockaddr_in recAddress;  // and receive from this. This is same among all handles of same protocol
	
	SOCKET send_sock;
	SOCKET recv_sock;
	unsigned long recvmessagecount;
	unsigned long sendmessagecount;
	unsigned long recvwouldblockcount;
	unsigned long sendwouldblockcount;
	
	HANDLE lock;
	HANDLE ThreadHandle;
	short ThreadActive;
	short current_get_index;
	short current_store_index;
	short last_gotten_index;
	short wrapped;
	char  *message_cache;
	short *bytes_recvd;
	struct comiphandle *parent;
	int referencecount;
	int BroadcastModeOn;
	int NeedBroadcastMode;
	//sfr: identifiers
	unsigned int whoami; // me, the host
	unsigned long id;    // id of the owner of this com
	unsigned long lastsender; // ip of last sender... kinda obvious. but will keep it here
	//sfr: converts 
	//port info
	unsigned long lastsenderport;
	// sfr id of last sender
	unsigned long lastsenderid;
	unsigned long timestamp;
	unsigned long *timestamps;
	unsigned long  *senders;
	Reliable_Data rudp_data;
} ComIP;


typedef struct comgrouphandle {
	struct comapihandle apiheader;
	int            buffer_size;
	unsigned int   HostID;
	CAPIList      *GroupHead;
	char          *send_buffer;
	int            max_header;
	char           TCP_buffer_shift;
	char           UDP_buffer_shift;
	char           RUDP_buffer_shift;
	char           DPLAY_buffer_shift;
} ComGROUP;


#define CAPI_SENDBUFSIZE 1024
#define CAPI_RECVBUFSIZE 65536

#ifdef __cplusplus
}
#endif
#endif
