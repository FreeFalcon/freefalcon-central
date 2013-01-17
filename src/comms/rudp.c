#pragma optimize( "", off ) // JB 010718

#pragma warning(disable : 4706)

#ifdef __cplusplus
extern "C" {
#endif


#include "capiopt.h"

#include <stdio.h>
#include <stdlib.h>
#include <winsock.h>
#include <assert.h>
#include <time.h>


#include "capi.h"
#include "capipriv.h"
#include "ws2init.h"
#include "wsprotos.h"
// sfr: new includes
#include "rudp.h"
#include "ComList.h" // new com list
#include "capibwcontrol.h"

// sfr: for new isbad checks
#include "falclib/include/isbad.h"

extern int ComAPILastError;
extern DWProc_t CAPI_TimeStamp;

extern int MonoPrint (char *, ...);

#define RUDPF_RESET	0x80
#define RUDPF_OOB	0x40
#define RUDPF_SEQ	0x20
#define RUDPF_LAST	0x10
#define RUDPF_LOOB	0x08
#define RUDPF_MSG	0x04

#define RUDP_RESEND_TIME		1500					// Time to wait for ack before resending
#define RUDP_OOB_RESEND_TIME	 750					// Time to wait for ack before resending an OOB message
#define RUDP_ACK_WAIT_TIME		 500					// Time to wait before acking
#define RUDP_OOB_ACK_WAIT_TIME	  50					// Time to wait before acking
#define RUDP_PING_TIME			2500					// Time to wait before pinging again

#define RUDP_RESET_REQ	0
#define RUDP_RESET_ACK	1
#define RUDP_RESET_OK	2
#define RUDP_PING		3
#define RUDP_PONG		4
#define RUDP_WORKING	5
#define RUDP_EXIT		6
#define RUDP_DROP		7

/* List head for connection list */
//extern CAPIList *GlobalListHead;
extern void enter_cs (void);
extern void leave_cs (void);

/* Mutex macros */
#define SAY_ON(a)            
#define SAY_OFF(a)			 
#define CREATE_LOCK(a,b)                { a = CreateMutex( NULL, FALSE, b ); if( !a ) DebugBreak(); }
#define REQUEST_LOCK(a)                 { int w = WaitForSingleObject(a, INFINITE); {SAY_ON(a);} if( w == WAIT_FAILED ) DebugBreak(); }
#define RELEASE_LOCK(a)                 { {SAY_OFF(a);} if( !ReleaseMutex(a)) DebugBreak();   }
#define DESTROY_LOCK(a)                 { if( !CloseHandle(a)) DebugBreak();   }


static struct sockaddr_in comRecvAddr;

/* forward function declarations */
void ComRUDPClose(ComAPIHandle c);
int ComRUDPSend(ComAPIHandle c, int msgsize, int oob, int type);
int ComRUDPSendX(ComAPIHandle c, int msgsize, int oob, int type, ComAPIHandle Xcom);
int ComRUDPGet(ComAPIHandle c);

int comms_compress (char *in, char *out, int size);
int comms_decompress (char *in, char *out, int size);
//extern int g_bF4CommsKillPL;

// sfr: test stuff
//extern int docomms;

int ComIPHostIDGet(ComAPIHandle c, char *buf, int reset);
char *ComRUDPSendBufferGet(ComAPIHandle c);
char *ComRUDPRecvBufferGet(ComAPIHandle c);
unsigned long ComRUDPQuery(ComAPIHandle c, int querytype);
unsigned long ComRUDPGetTimeStamp(ComAPIHandle c);

#define GETActiveCOMHandle(c)   (((ComIP *)c)->parent == NULL) ? ((ComIP *)c) : ((ComIP *)c)->parent

CAPIList * CAPIListAppend( CAPIList * list );
CAPIList * CAPIListRemove( CAPIList * list ,ComAPIHandle c);
CAPIList * CAPIListAppendTail( CAPIList * list );

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int get_rudp_max_queue_length (void){
	int max, count;

	Reliable_Packet *rp;

	ComIP *curr;

	max = 0;

	enter_cs(); // JPO
	//sfr new list
	for (curr = comListGetFirstP(CAPI_RUDP_PROTOCOL); curr != NULL; curr = comListGetNextP(CAPI_RUDP_PROTOCOL)){
		count = 0;
		rp = curr->rudp_data.sending;
		while (rp) {
			count ++;
			rp = rp->next;
		}
		if (count > max){
			max = count;
		}
	}

	leave_cs();
	return max;
}

ComAPIHandle ComRUDPOpenSendClone(
	char *name_in,
	ComIP *parentCom,
	int buffersize, 
	char *gamename, 
	int rudpPort,
	unsigned long IPaddress,
	unsigned long id
){
	ComIP *c;
	
	c = (ComIP*)malloc(sizeof(ComIP));
	//GlobalListHead->com = (ComAPIHandle)c;
	memset(c,0,sizeof(ComIP));
	
	memcpy(c,parentCom,sizeof(ComIP));
	
	((ComAPIHandle)c)->name = (char*)malloc (strlen (name_in) + 1);
	strcpy (((ComAPIHandle)c)->name, name_in);
	
	/* initialize header data */
	
	c->parent = parentCom;
	parentCom->referencecount++;
	c->referencecount = 1;
	
	c->buffer_size = max(parentCom->buffer_size, (int)(sizeof(ComAPIHeader) + buffersize + 16));
	
	if ((c->max_buffer_size > 0) && (c->buffer_size > c->max_buffer_size))
	{
		c->buffer_size = c->max_buffer_size;
	}
	
	c->send_buffer.buf = (char *)malloc(c->buffer_size);
	c->rudp_data.real_send_buffer = (char *)malloc(c->buffer_size);

	c->rudp_data.sequence_number = 0;		/* sequence number for sending */
	c->rudp_data.oob_sequence_number = 0;		/* sequence number for sending */
	c->rudp_data.message_number = 0;		/* message number for sending */
	c->rudp_data.sending = 0;				/* list of sent packets */
	c->rudp_data.oob_sending = 0;				/* list of sent packets */
	c->rudp_data.last_sent = 0;				/* list of sent packets */
	c->rudp_data.oob_last_sent = 0;				/* list of sent packets */

	c->rudp_data.reset_send = 0;			/* What reset stage are we in */

	c->rudp_data.send_ack = 0;
	c->rudp_data.send_oob_ack = 0;
	c->rudp_data.last_sequence = 0;			/* other's last seen sequence number */
	c->rudp_data.last_oob_sequence = 0;			/* other's last seen sequence number */
	c->rudp_data.last_received = 0;			/* my last received sequential sequence number for ack.*/
	c->rudp_data.last_oob_received = 0;			/* my last received sequential sequence number for ack.*/
	c->rudp_data.last_sent_received = 0;	/* the last last_received that I acknowledged */
	c->rudp_data.last_oob_sent_received = 0;	/* the last last_received that I acknowledged */

	c->rudp_data.last_dispatched = 0;		/* the last dispatched message */
	c->rudp_data.last_oob_dispatched = 0;		/* the last dispatched message */

	c->rudp_data.receiving = 0;				/* list of received packets */
	c->rudp_data.oob_receiving = 0;				/* list of received packets */

	c->rudp_data.sent_received = 0;			/* what the last received I sent */
	c->rudp_data.sent_oob_received = 0;			/* what the last received I sent */
	c->rudp_data.last_send_time = 0;		/* last time we checked for ack */
	c->rudp_data.last_oob_send_time = 0;		/* last time we checked for ack */

	c->rudp_data.last_ping_send_time = 0;
	c->rudp_data.last_ping_recv_time = GetTickCount ();
	
	c->compression_buffer = (char *)malloc(c->buffer_size);
	
	ComIPHostIDGet(&c->apiheader, c->send_buffer.buf, 0);
	
	strncpy(((ComAPIHeader *)c->rudp_data.real_send_buffer)->gamename, gamename, GAME_NAME_LENGTH);
	//sfr: id network order
	((ComAPIHeader *)c->rudp_data.real_send_buffer)->id = c->whoami;

	// sfr: get ID instead of IP if IP is flag
	if (IPaddress == CAPI_DANGLING_IP){
		ComIPHostIDGet(&c->apiheader, (char*)&IPaddress, 0);
		IPaddress = CAPI_htonl (IPaddress);
	}

	/* Outgoing... */
	memset ((char*)&c->sendAddress, 0, sizeof(c->sendAddress));
	c->sendAddress.sin_family       = AF_INET;
	c->sendAddress.sin_addr.s_addr  = CAPI_htonl(IPaddress);
	c->sendAddress.sin_port         = CAPI_htons((unsigned short)rudpPort);

	// sfr: id (store network order)
	c->id = CAPI_htonl(id);

	comListAdd(c);
	return (ComAPIHandle)c;
}



/* begin a comms session 
* sfr: TODO this function is not freeing anything on error
*/
ComAPIHandle ComRUDPOpen(
	char *name_in, 
	int buffersize, 
	char *gamename, 
	unsigned short localPort,
	unsigned short remotePort,
	unsigned long IPaddress,
	unsigned long id,
	int idealpacketsize
){
	ComIP *c;
	unsigned long trueValue = 1;
	int	err, size;
	//CAPIList *listitem, *curr = 0;
	ComIP *comRUDP;
	WSADATA wsaData;

	// capi critical section 
	enter_cs ();
	// init socket stuff
	if (InitWS2(&wsaData) == 0) {
		leave_cs ();
		return 0;
	}
	
	// we already setup comms, use a clone
	comRUDP = comListFindProtocolRport(CAPI_RUDP_PROTOCOL, localPort);
	if (comRUDP != NULL){
		ComAPIHandle ret_val;
		ret_val = ComRUDPOpenSendClone(name_in, comRUDP, buffersize, gamename, remotePort, IPaddress, id);
		leave_cs ();
		return ret_val;
	}
	
	// allocate and zero
	c = (ComIP*)malloc(sizeof(ComIP));
	memset(c, 0, sizeof(ComIP));
	((ComAPIHandle)c)->name = strdup(name_in);


	// sfr: get ID instead of IP if IP is flag
	if (IPaddress == CAPI_DANGLING_IP){
		ComIPHostIDGet(&c->apiheader, (char*)&IPaddress, 0);
		IPaddress = CAPI_htonl (IPaddress);
	}

	// initialize header data
	c->apiheader.protocol = CAPI_RUDP_PROTOCOL;
	c->apiheader.send_func = ComRUDPSend;
	c->apiheader.sendX_func = ComRUDPSendX;
	c->apiheader.recv_func = ComRUDPGet;
	c->apiheader.send_buf_func = ComRUDPSendBufferGet;
	c->apiheader.recv_buf_func = ComRUDPRecvBufferGet;
	c->apiheader.addr_func = ComIPHostIDGet;
	c->apiheader.close_func = ComRUDPClose;
	c->apiheader.query_func = ComRUDPQuery;
	c->apiheader.get_timestamp_func = ComRUDPGetTimeStamp;
	c->sendmessagecount = 0;
	c->recvmessagecount = 0;
	c->recvwouldblockcount = 0;
	c->sendwouldblockcount = 0;
	c->referencecount = 1;
	c->timestamp = 0;
	c->max_buffer_size = 0;
	c->ideal_packet_size = idealpacketsize;
	
	memset (&c->rudp_data, 0, sizeof (Reliable_Data));
	
	c->recv_sock = CAPI_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	size = CAPI_SENDBUFSIZE;
	CAPI_setsockopt (c->recv_sock, SOL_SOCKET, SO_SNDBUF, (char*) &size, 4);
	size = CAPI_RECVBUFSIZE;
	CAPI_setsockopt (c->recv_sock, SOL_SOCKET, SO_RCVBUF, (char*) &size, 4);
	
	CAPI_ioctlsocket(c->recv_sock, FIONBIO, &trueValue);

	c->buffer_size = sizeof(ComAPIHeader) + buffersize;
	
	if ((c->max_buffer_size > 0) && (c->buffer_size  > c->max_buffer_size))
	{
		c->buffer_size = c->max_buffer_size;
	}

	c->recv_buffer.buf = (char *)malloc(c->buffer_size);
	c->recv_buffer.len = c->buffer_size;
	c->send_buffer.buf = (char *)malloc(c->buffer_size);
	c->rudp_data.real_send_buffer = (char *)malloc(c->buffer_size);
	
	c->compression_buffer = (char *)malloc(c->buffer_size);
	
	//sfr: store as network. function returns host
	ComIPHostIDGet(&c->apiheader, (char *)&c->whoami, 0);
	c->whoami = CAPI_htonl(c->whoami);

	
	strncpy(((ComAPIHeader *)c->rudp_data.real_send_buffer)->gamename, gamename, GAME_NAME_LENGTH);
	//sfr id network order
	((ComAPIHeader *)c->rudp_data.real_send_buffer)->id = c->whoami;

	CAPI_ioctlsocket(c->recv_sock, FIONBIO, &trueValue);

	if(c->recv_sock == INVALID_SOCKET) {
		leave_cs ();
		return 0;
	}
	
	// Incoming...
	memset ((char*)&comRecvAddr, 0, sizeof(comRecvAddr));
	comRecvAddr.sin_family       = AF_INET;
	comRecvAddr.sin_addr.s_addr  = CAPI_htonl(INADDR_ANY);
	comRecvAddr.sin_port         = CAPI_htons(localPort);
	memcpy(&c->recAddress, &comRecvAddr, sizeof(struct sockaddr_in));

	err = CAPI_bind(c->recv_sock, (struct sockaddr*)&comRecvAddr,sizeof(comRecvAddr));
	if (err) {
		leave_cs ();
		return 0;
	}
	
	//c->send_sock = CAPI_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	c->send_sock = c->recv_sock;
	
	size = CAPI_SENDBUFSIZE;
	CAPI_setsockopt (c->send_sock, SOL_SOCKET, SO_SNDBUF, (char*) &size, 4);
	size = CAPI_RECVBUFSIZE;
	CAPI_setsockopt (c->send_sock, SOL_SOCKET, SO_RCVBUF, (char*) &size, 4);
	
	if (c->send_sock == INVALID_SOCKET) {
		leave_cs ();
		return 0;
	}
	/**  .. on ISPs modem maybe better to set to Non-Blocking on send **/
	CAPI_ioctlsocket(c->send_sock, FIONBIO, &trueValue);
	
	/* Outgoing... */
	memset ((char*)&c->sendAddress, 0, sizeof(c->sendAddress));
	c->sendAddress.sin_family       = AF_INET;
	c->sendAddress.sin_addr.s_addr  = CAPI_htonl(IPaddress);
	c->sendAddress.sin_port         = CAPI_htons(remotePort);
	
	c->rudp_data.sequence_number = 0;		/* sequence number for sending */
	c->rudp_data.oob_sequence_number = 0;		/* sequence number for sending */
	c->rudp_data.message_number = 0;		/* message number for sending */
	c->rudp_data.sending = 0;				/* list of sent packets */
	c->rudp_data.oob_sending = 0;				/* list of sent packets */
	c->rudp_data.last_sent = 0;				/* list of sent packets */
	c->rudp_data.oob_last_sent = 0;				/* list of sent packets */

	c->rudp_data.reset_send = 0;			/* I need to get a S0 or I send a reset */

	c->rudp_data.send_ack = 0;
	c->rudp_data.send_oob_ack = 0;
	c->rudp_data.last_sequence = 0;			/* other's last seen sequence number */
	c->rudp_data.last_oob_sequence = 0;			/* other's last seen sequence number */
	c->rudp_data.last_received = 0;			/* my last received sequential sequence number for ack.*/
	c->rudp_data.last_oob_received = 0;			/* my last received sequential sequence number for ack.*/
	c->rudp_data.last_sent_received = 0;	/* the last last_received that I acknowledged */
	c->rudp_data.last_oob_sent_received = 0;	/* the last last_received that I acknowledged */

	c->rudp_data.last_dispatched = 0;		/* the last dispatched message */
	c->rudp_data.last_oob_dispatched = 0;		/* the last dispatched message */

	c->rudp_data.receiving = 0;				/* list of received packets */
	c->rudp_data.oob_receiving = 0;				/* list of received packets */

	c->rudp_data.sent_received = 0;			/* what the last received I sent */
	c->rudp_data.sent_oob_received = 0;			/* what the last received I sent */
	c->rudp_data.last_send_time = 0;		/* last time we checked for ack */
	c->rudp_data.last_oob_send_time = 0;		/* last time we checked for ack */

	c->rudp_data.last_ping_send_time = 0;
	c->rudp_data.last_ping_recv_time = GetTickCount ();

	// sfr: id (store network order)
	c->id = CAPI_htonl(id);

	// sfr add to com list
	comListAdd(c);
	leave_cs ();
	return (ComAPIHandle)c;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* get the associated write buffer for RUDP*/
char *ComRUDPSendBufferGet(ComAPIHandle c)
{
	return ((ComIP *)c)->send_buffer.buf;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char *ComRUDPRecvBufferGet(ComAPIHandle c)
{
	return ((ComIP *)c)->recv_buffer.buf;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ComRUDPSendX(ComAPIHandle c, int msgsize, int oob, int type, ComAPIHandle Xcom)
{
	if(c == Xcom){
		return 0;
	}
	else {
		return ComRUDPSend(c, msgsize, oob, type);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int send_rudp_packet (ComIP *cudp, Reliable_Packet *rp, int type)
{
	unsigned char *ptr, *flags; 
	
	int newsize, size, sent = 0;

	long now = GetTickCount();
	// We're copying all our data into the temp_buffer
	ptr = (unsigned char *)cudp->rudp_data.real_send_buffer;
	
	// Every message needs a ComAPIHeader (it never gets written over for rudp)
	ptr += sizeof(ComAPIHeader);
	size = sizeof(ComAPIHeader);
	
	// Every message needs a flags field
	flags = ptr;
	*flags = 0;
	ptr++;
	size++;

	if (cudp->rudp_data.reset_send <= RUDP_RESET_OK){
		if (now - cudp->rudp_data.last_send_time > RUDP_OOB_RESEND_TIME){
			*flags = RUDPF_RESET;
			*flags |= cudp->rudp_data.reset_send;
			cudp->rudp_data.last_send_time = now;
		}
	}
	else if (cudp->rudp_data.reset_send == RUDP_RESET_OK){
		*flags = RUDPF_RESET;
		*flags |= cudp->rudp_data.reset_send;
		cudp->rudp_data.last_send_time = now;
	}
	else if (cudp->rudp_data.reset_send == RUDP_PING){
		*flags = RUDPF_RESET | RUDP_PING;
		*(long *)ptr = now;
		ptr += sizeof (long);
		size += sizeof (long);
	}
	else if (cudp->rudp_data.reset_send == RUDP_EXIT){
		*flags = RUDPF_RESET | RUDP_EXIT;
	}
	else if (cudp->rudp_data.reset_send == RUDP_DROP){
		*flags = RUDPF_RESET | RUDP_DROP;
	}
	else {
		// If our last_sent_received is out of date, we need to send it
		if (cudp->rudp_data.last_sent_received != cudp->rudp_data.last_received){
			cudp->rudp_data.send_ack = FALSE;
			*flags |= RUDPF_LAST;
			*(unsigned short*)ptr = (unsigned short)cudp->rudp_data.last_received;

			cudp->rudp_data.last_sent_received = cudp->rudp_data.last_received;
			ptr += sizeof(short);
			size += sizeof(short);
		}
		
		// If our last_sent_received is out of date, we need to send it
		if (cudp->rudp_data.last_oob_sent_received != cudp->rudp_data.last_oob_received){
			cudp->rudp_data.send_oob_ack = FALSE;
			*flags |= RUDPF_LOOB;
			*(unsigned short*)ptr = (unsigned short)cudp->rudp_data.last_oob_received;

			cudp->rudp_data.last_oob_sent_received = cudp->rudp_data.last_oob_received;
			ptr += sizeof(short);
			size += sizeof(short);
		}
		
//		cp = cudp->rudp_data.receiving;
//		count = 0;
//		
//		while (cp)
//		{
//			if ((!cp->acknowledged) && ((cudp->rudp_data.last_received - cp->sequence_number + 1) & 0x8000))
//			{
//				cudp->rudp_data.send_ack = FALSE;
//
//				count ++;
//				cp->acknowledged = TRUE;
//				// *flags |= RUDPF_ACK;
//				*(unsigned short*)ptr = cp->sequence_number;
//				ptr += sizeof (short);
//				size += sizeof (short);
//			}
//			
//			if (count >= 7)
///			{
//				break;
//			}
//
//			cp = cp->next;
//		}
//		
//		if (count)
//		{
//			*flags |= (count);
//		}
		
		// If we're sending a packet, we have a sequence number & data
		if (rp)
		{
			*flags |= RUDPF_SEQ;
			*(unsigned short*)ptr = rp->sequence_number;
			ptr += sizeof(short);
			size += sizeof(short);

			if (rp->oob)
			{
				*flags |= RUDPF_OOB;
			}
			
			// If we're a packetized message, send a message id, slot, etc.
			if (rp->message_parts > 1)
			{
				*flags |= RUDPF_MSG;
				*(unsigned short*)ptr = rp->message_number;
				ptr += sizeof(unsigned short);
				size += sizeof(unsigned short);
				*ptr = rp->message_slot;
				ptr ++;
				size ++;
				*ptr = rp->message_parts;
				ptr ++;
				size ++;
			}

			assert(FALSE == IsBadWritePtr(ptr, rp->size));
			//if (!IsBadWritePtr(ptr, sizeof(unsigned char))) // JB 010223 CTD
			if (!IsBadWritePtr(ptr, rp->size)) // JB 010401 CTD
				memcpy (ptr, rp->data, rp->size);
			size += rp->size;
			assert(size < cudp->buffer_size);

//			MonoPrint ("Send RUDP Packet %08x %08x\n", cudp->address.sin_addr.s_addr, size);
		}
	}
	
	// If we have something worth sending
	if (*flags){
		if (size < 32){
			newsize = size;
		}
		else{
			memcpy(cudp->compression_buffer, &size, sizeof(u_short));
			memset (cudp->rudp_data.real_send_buffer + size, 0, cudp->buffer_size - size);
			newsize = comms_compress (cudp->rudp_data.real_send_buffer, cudp->compression_buffer+sizeof(u_short), size);
		}

		// sfr: test comms drop
		sent = 0;
		//if (docomms){
		if ((int)(newsize + sizeof (u_short)) < (int) (size))
		{
			sent = CAPI_sendto
			(
				cudp->send_sock,
				cudp->compression_buffer,
				newsize + sizeof (u_short),
				0,
				(struct sockaddr *)&cudp->sendAddress,   
				sizeof(cudp->sendAddress)
			);
		}
		else
		{
			sent = CAPI_sendto
			(
				cudp->send_sock,
				cudp->rudp_data.real_send_buffer,
				size,
				0,
				(struct sockaddr *)&cudp->sendAddress,   
				sizeof(cudp->sendAddress)
			);
		}
		//}
		
		if (sent > 0){
			cudp->rudp_data.last_ping_send_time = now;

			if (rp){
				if (rp->oob){
					cudp->rudp_data.last_oob_send_time = now;
				}
				else{
					cudp->rudp_data.last_send_time = now;
				}
				rp->last_sent_at = now;
			}
			use_bandwidth (sent, 1, type);
		}
        else if (size == SOCKET_ERROR)
		{
			int senderror;
			senderror = CAPI_WSAGetLastError();
			switch(senderror)
			{
				case WSAEWOULDBLOCK:
				{
					// if (rp)
					// {
					//	MonoPrint ("WouldBlock %d %d %d\n", size, get_bandwidth_available (), rp->sequence_number);
					// }
					// else
					// {
					// 	MonoPrint ("WouldBlock %d %d\n", size, get_bandwidth_available ());
					// }
					cut_bandwidth ();
					cudp->sendwouldblockcount++;
					/* The socket is marked as non-blocking and the send
					operation would block. */
					return COMAPI_WOULDBLOCK;
				}

				default :
				{
					return 0;
				}
			}
		}
		
		if (*flags & RUDPF_RESET)
		{
			if (size)
			{
				return -2;
			}
			else
			{
				return 0;
			}
		}
		
		return size;
	}
	else
	{
		return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* send data from a comms session */
int ComRUDPSend(ComAPIHandle c, int msgsize, int oob, int type){
	Reliable_Packet *pp, *lp, *rp;
	
	int time, count, left, parts, slot=0, offset=0;
	long now;

	if (c){
		ComIP *cudp = (ComIP *)c;
		// Check for exceeding buffer
		if (msgsize > cudp->buffer_size){
			return COMAPI_MESSAGE_TOO_BIG;
		}

		if (msgsize){
			count = 0;
			lp = cudp->rudp_data.sending;
			// sfr: TODO remove JB check

			while (lp && !F4IsBadReadPtrC(lp, sizeof(Reliable_Packet))){
				count ++;
				lp = lp->next;
			}
			
			lp = cudp->rudp_data.oob_sending;

			while (lp && !F4IsBadReadPtrC(lp, sizeof(Reliable_Packet))){ // JB 010220 CTD
				count ++;
				lp = lp->next;
			}

			if (count > 0x2000){
				return COMAPI_WOULDBLOCK;
			}
			
			// Packetize, or create a single reliable packet
			// NOTE: we may want to packetize only if the additional packets are reasonably sized
			left = msgsize;
			parts = 1 + ((msgsize-1) / (cudp->ideal_packet_size - MAX_RUDP_HEADER_SIZE));
			if (parts > 1){
				cudp->rudp_data.message_number++;
				if (!cudp->rudp_data.message_number){
					cudp->rudp_data.message_number = 1;
				}
			}

			while (left > 0){
				// Make a new packet
				rp = (Reliable_Packet *) malloc (sizeof(Reliable_Packet));
				rp->dispatched = FALSE;
				rp->send_count = 0;
				rp->oob = (unsigned char)oob;
				rp->acknowledged = FALSE;

				if (oob){
					cudp->rudp_data.oob_sequence_number = (unsigned short)((cudp->rudp_data.oob_sequence_number + 1) & 0xffff);
					rp->sequence_number = cudp->rudp_data.oob_sequence_number;
				}
				else{
					cudp->rudp_data.sequence_number = (unsigned short)((cudp->rudp_data.sequence_number + 1) & 0xffff);
					rp->sequence_number = cudp->rudp_data.sequence_number;
				}

				if (parts > 1){
					rp->message_number = cudp->rudp_data.message_number;
				}
				else{
					rp->message_number = 0;
				}

				rp->message_slot = (unsigned char)slot;
				rp->message_parts = (unsigned char)parts;
				rp->last_sent_at = 0;

				if (left > cudp->ideal_packet_size - (int)MAX_RUDP_HEADER_SIZE){
					rp->size = (unsigned short)(cudp->ideal_packet_size - MAX_RUDP_HEADER_SIZE);
				}
				else{
					rp->size = (unsigned short)left;
				}

				rp->data = (char*)malloc (rp->size);
				assert (rp->data);
				assert (cudp->buffer_size >= (signed int) (offset  + rp->size)); // JPO check length
				memcpy (rp->data, cudp->send_buffer.buf+offset, rp->size);

				// Increment our sizes
				left -= rp->size;
				offset += rp->size;
				slot++;
				
				// Add the new packet to our sending list

				if (oob){
					lp = cudp->rudp_data.oob_sending;
				}
				else{
					lp = cudp->rudp_data.sending;
				}

				pp = NULL;
				if (lp){
					if (oob){
						//if (cudp->rudp_data.oob_last_sent) // JB 010221 CTD
						if (cudp->rudp_data.oob_last_sent && !F4IsBadReadPtrC(cudp->rudp_data.oob_last_sent, sizeof(Reliable_Packet))) // JB 010221 CTD
						{
							cudp->rudp_data.oob_last_sent->next = rp;
						}

						cudp->rudp_data.oob_last_sent = rp;
						rp->next = NULL;
					}
					else{
						//if (cudp->rudp_data.last_sent) // JB 010221 CTD
						if (cudp->rudp_data.last_sent && !F4IsBadReadPtrC(cudp->rudp_data.last_sent, sizeof(Reliable_Packet))) // JB 010221 CTD
						{
							cudp->rudp_data.last_sent->next = rp;
						}

						cudp->rudp_data.last_sent = rp;
						rp->next = NULL;
					}
				}
				else{
					if (oob){
						cudp->rudp_data.oob_sending = cudp->rudp_data.oob_last_sent = rp;
						rp->next = NULL;
					}
					else{
						cudp->rudp_data.sending = cudp->rudp_data.last_sent = rp;
						rp->next = NULL;
					}
				}
			}
		}
		else{
			// If any of our sending packets are timed out
			// and haven't been ACK'd send them now
			now = GetTickCount();
			lp = cudp->rudp_data.oob_sending;
			time = RUDP_OOB_RESEND_TIME;

			while (lp){
				if ((!lp->acknowledged) && ((int)(now - lp->last_sent_at) > time)) // && ((lp->sequence_number - cudp->rudp_data.last_sequence - 8) & 0x8000))
				{
					lp->send_count ++;
					
					if (/*(lp->oob) || */(check_bandwidth(lp->size, 1, type))){
						return send_rudp_packet (cudp, lp, type);
					}
					// sfr: we dont have this anymore
					// TODO maybe we have to reserve its now
					//else
					//{
					//	rudp_bandwidth (lp->size * 2);
					//}
					return 0;
				}

				lp = lp->next; // - only used if we change the "if (lp)" to a while
			}

			now = GetTickCount();
			lp = cudp->rudp_data.sending;
			time = RUDP_RESEND_TIME;

			while (lp){
				if ((!lp->acknowledged) && ((int)(now - lp->last_sent_at) > time)) // && ((lp->sequence_number - cudp->rudp_data.last_sequence - 8) & 0x8000))
				{
					lp->send_count ++;
					
					if (/*(lp->oob) || */(check_bandwidth(lp->size, 1, type))){
						return send_rudp_packet (cudp, lp, type);
					}
					// sfr: we dont have this anymore
					// maybe we need to reserve rudp bw
					//else
					//{
					//	rudp_bandwidth (lp->size * 2);
					//}

					return 0;
				}

				lp = lp->next; // - only used if we change the "if (lp)" to a while
			}

			return 0;
		}
		
		cudp->sendmessagecount++;
		return msgsize;
	}
	else{
		// no comm handle
		return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Returns size of message added to queue
static int add_to_receive_queue(ComIP *cudp, unsigned char *ptr, int size)
{
	int count;
//		bytesRecvd=-1;
	
//	unsigned short
//		ack;
	
	unsigned char flags;
	
	Reliable_Packet *cp, *lp, *np, *rp;
	
	// Trim off the flags
	flags = *ptr;
	ptr ++;
	size --;
	
	cudp->rudp_data.last_ping_recv_time = GetTickCount ();

	if (flags & RUDPF_RESET)
	{
//		MonoPrint ("Recv %d\n", flags & RUDPF_MASK);

		switch (flags & 0x0f)
		{
			case RUDP_RESET_REQ:
			{
				if (cudp->rudp_data.reset_send == RUDP_WORKING)
				{
					cudp->rudp_data.reset_send = RUDP_EXIT;
				}
				
				cudp->rudp_data.reset_send = RUDP_RESET_ACK;
				cudp->rudp_data.last_send_time = 0;
				cudp->rudp_data.last_oob_send_time = 0;
				send_rudp_packet (cudp, NULL, 0);
				send_rudp_packet (cudp, NULL, 0);
				send_rudp_packet (cudp, NULL, 0);
				break;
			}

			case RUDP_RESET_ACK:
			{
				cudp->rudp_data.reset_send = RUDP_RESET_OK;
				cudp->rudp_data.last_send_time = 0;
				cudp->rudp_data.last_oob_send_time = 0;
				send_rudp_packet (cudp, NULL, 0);
				send_rudp_packet (cudp, NULL, 0);
				send_rudp_packet (cudp, NULL, 0);
				cudp->rudp_data.reset_send = RUDP_WORKING;
				break;
			}

			case RUDP_RESET_OK:
			{
				cudp->rudp_data.reset_send = RUDP_WORKING;
				break;
			}

			case RUDP_PING:
			{
				// This is just sent to fill up some bandwidth
				// the code below is done for all received packets

				//cudp->rudp_data.last_ping_recv_time = GetTickCount ();

				break;
			}

			case RUDP_EXIT:
			{
				cudp->rudp_data.reset_send = RUDP_EXIT;
				cudp->rudp_data.last_send_time = 0;
				cudp->rudp_data.last_oob_send_time = 0;
				send_rudp_packet (cudp, NULL, 0);
				break;
			}

			case RUDP_DROP:
			{
				if (cudp->rudp_data.reset_send == RUDP_WORKING)
				{
					cudp->rudp_data.reset_send = RUDP_DROP;
				}
				break;
			}
		}

		return 0;
	}

	if (cudp->rudp_data.reset_send != RUDP_WORKING)
	{
		return 0;
	}

	count = 0;
	if (flags & RUDPF_LAST)
	{
		count += 2;
	}

	if (flags & RUDPF_LOOB)
	{
		count += 2;
	}

//	if (flags & RUDPF_MASK)
//	{
//		count += 2 * (flags & RUDPF_MASK);
//	}
	// 2003-09-27 Fix by Will Mulvihill - RUDPF_SEQ is a 2 byte message - see above line 694 - size = short = 2...
	if (flags & RUDPF_SEQ)
	{
		//count += 4;
		count += 2;
	}

	if (flags & RUDPF_MSG)
	{
		count += 4;
	}

	if (count > size)
	{
		return 0;
	}

	// Trim off last_sequence number received by remote
	if (flags & RUDPF_LAST)
	{
		cudp->rudp_data.last_sequence = (*(unsigned short *) ptr) & 0xffff;
		ptr += sizeof(short);
		size -= sizeof(short);
	}
	
	if (flags & RUDPF_LOOB)
	{
		cudp->rudp_data.last_oob_sequence = (*(unsigned short *) ptr) & 0xffff;
		ptr += sizeof(short);
		size -= sizeof(short);
	}
	
//	if (flags & RUDPF_MASK)
//	{
//		count = flags & RUDPF_MASK;
//		
//		while (count)
//		{
//			ack = *(short *) ptr;
//			
//			cp = cudp->rudp_data.sending;
//			
//			while (cp)
//			{
//				if (cp->sequence_number == ack)
//				{
//					cp->acknowledged = TRUE;
//					break;
//				}
//				
//				cp = cp->next;
//			}
//			
//			ptr += sizeof (short);
//			size -= sizeof (short);
//			
//			count --;
//		}
//	}
	
	// Trim off the sequence number
	if (flags & RUDPF_SEQ)
	{
		// Fail early if we've seen this packet before
		if (flags & RUDPF_OOB)
		{
			if ((*(unsigned short*)ptr - ((cudp->rudp_data.last_oob_received + 1) & 0xffff)) & 0x8000)
			{
				cudp->rudp_data.last_oob_sent_received = -1;
				return size;
			}

			cudp->rudp_data.send_oob_ack = TRUE;
		}
		else
		{
			if ((*(unsigned short*)ptr - ((cudp->rudp_data.last_received + 1) & 0xffff)) & 0x8000)
			{
				cudp->rudp_data.last_sent_received = -1;
				return size;
			}

			cudp->rudp_data.send_ack = TRUE;
		}

		rp = malloc(sizeof(Reliable_Packet));
		rp->sequence_number = *(unsigned short*)ptr;
		rp->dispatched = FALSE;
		rp->acknowledged = FALSE;
		rp->send_count = 0;
		rp->data = NULL;
		rp->next = NULL;

		if (flags & RUDPF_OOB)
		{
			rp->oob = TRUE;
		}
		else
		{
			rp->oob = FALSE;
		}

		ptr += sizeof(short);
		size -= sizeof(short);
	}
	else
	{
		rp = NULL;
	}
	
	// Trim off the message data, if any
	if (flags & RUDPF_MSG)
	{
		assert (rp);
		rp->message_number = *(unsigned short *)ptr;
		ptr += sizeof(unsigned short);
		size -= sizeof(unsigned short);
		rp->message_slot = *ptr;
		ptr ++;
		size --;
		rp->message_parts = *ptr;
		ptr ++;
		size --;
	}
	else if (rp)
	{
		rp->message_number = 0;
		rp->message_slot = 0;
		rp->message_parts = 1;
	}
	
	// Trim off the data
	if (flags & RUDPF_SEQ)
	{
		assert (rp);
		rp->size = (unsigned short)size;
		rp->data = (char*)malloc(size);
		memcpy(rp->data, ptr, size);
		
		if (CAPI_TimeStamp)
		{
			cudp->timestamp = CAPI_TimeStamp();
		}
	}
	else
	{
		assert (size == 0);
		return 0;
	}
	
	if (!size || !rp)
	{
		return 0;
	}
	
	// Add the new packet to the receiving queue at the right place
	lp = np = NULL;
	if (rp->oob)
	{
		cp = cudp->rudp_data.oob_receiving;
	}
	else
	{
		cp = cudp->rudp_data.receiving;
	}

	while (cp)
	{
		// Don't store the packet if we have already seen it, or its before
		// first known good packet
		if (cp->sequence_number == rp->sequence_number)
		{
			// since we have seen it before, make sure we resend the last received numbers
			// or acknowledge this packet
			cp->acknowledged = FALSE;

			if (rp->oob)
			{
				cudp->rudp_data.last_oob_sent_received = -1;
			}
			else
			{
				cudp->rudp_data.last_sent_received = -1;
			}

			free(rp);
			rp = NULL;
			return size;
		}
		
		// otherwise insert it into the receiving queue.
		if ((rp->sequence_number - cp->sequence_number) & 0x8000)
		{
			if (lp)
			{
				lp->next = rp;
			}
			else
			{
				if (rp->oob)
				{
					cudp->rudp_data.oob_receiving = rp;
				}
				else
				{
					cudp->rudp_data.receiving = rp;
				}
			}

			rp->next = cp;
			break;
		}
		
		lp = cp;
		cp = cp->next;
	}
	
	// if the queue is empty or we're at the end of the list, insert it
	if (!cp)
	{
		if (lp)
		{
			lp->next = rp;
		}
		else
		{
			if (rp->oob)
			{
				cudp->rudp_data.oob_receiving = rp;
			}
			else
			{
				cudp->rudp_data.receiving = rp;
			}
		}

		rp->next = NULL;
	}
	
	cudp->recvmessagecount++;
	
	return size;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int get_rudp_packets (ComIP *cudp)
{
	int
		size,
		got_packets=0,
		bytesRecvd=-1,
		recverror;
	
	struct sockaddr
		in_addr;
	
	struct in_addr
		addr;
	
	unsigned char
		*ptr;

	unsigned long id; // id of message owner
	
	ComIP *curr;
	//CAPIList 
	//	*curr;
	
	// Loop until we're out of stuff to read
	while (bytesRecvd)
	{
		cudp->recv_buffer.len = cudp->buffer_size;
		
		size =  sizeof(in_addr);	  
		
		bytesRecvd = CAPI_recvfrom
		(
			cudp->recv_sock,
			cudp->compression_buffer,
			cudp->recv_buffer.len,
			0,
			&in_addr,
			&size
		);

		if (bytesRecvd == SOCKET_ERROR)
		{
			recverror = CAPI_WSAGetLastError();
			
			switch(recverror)
			{
				case WSANOTINITIALISED:
				case WSAENETDOWN:
				case WSAEFAULT:
				case WSAENOTCONN:
				case WSAEINTR:
				case WSAEINPROGRESS:
				case WSAENETRESET:
				case WSAENOTSOCK:
				case WSAEOPNOTSUPP:
				case WSAESHUTDOWN:
					return 0;

				case WSAEWOULDBLOCK:
					cudp->recvwouldblockcount++;
					return 0;

				case WSAEMSGSIZE:
				case WSAEINVAL:
				case WSAECONNABORTED:
				case WSAETIMEDOUT:
				case WSAECONNRESET:
				default :
					return 0;
			}
		}
		
		if (bytesRecvd == 0)
		{
			return 0;
		}

		if (*(u_short*)cudp->compression_buffer <= 700)
		{
			memcpy(&size, cudp->compression_buffer, sizeof(u_short));
			comms_decompress (cudp->compression_buffer + sizeof(u_short), cudp->recv_buffer.buf, size);
			cudp->recv_buffer.len = size;
			bytesRecvd = size;
		}
		else
		{
			memcpy (cudp->recv_buffer.buf, cudp->compression_buffer, bytesRecvd);
			cudp->recv_buffer.len = bytesRecvd;
		}

		/*// sfr: added port info
		if (
			(((struct sockaddr_in *)(&in_addr))->sin_addr.s_addr == cudp->whoami) &&
			((struct sockaddr_in *)(&in_addr))->sin_port == CAPI_htons(ComAPIGetMySendPort())
		){*/
		id = ((ComAPIHeader *)cudp->recv_buffer.buf)->id;
		if (id == cudp->whoami){
			/* From ourself */
			continue;
		}
		//sfr: query stuff
		// moved below
		//cudp->lastsender = ((struct sockaddr_in*)&in_addr)->sin_addr.s_addr;
		//cudp->lastsenderport = ((struct sockaddr_in*)&in_addr)->sin_port;

		size = bytesRecvd;
		ptr = (unsigned char *)cudp->recv_buffer.buf;
		{
			ComAPIHeader *ch = (ComAPIHeader *)ptr;		
			// Trim off ComAPIHeader
			if (strncmp(ch->gamename, ((ComAPIHeader *)cudp->rudp_data.real_send_buffer)->gamename, GAME_NAME_LENGTH))
			{
				/* Not a good header */
				continue;
			}
		}		
		ptr += sizeof(ComAPIHeader);
		size -= sizeof(ComAPIHeader);
		
		// Find who sent it and add to their queue
		addr = ((struct sockaddr_in*)(&in_addr))->sin_addr;
		enter_cs(); // JPO
		// sfr: chicken egg prob
		// try to match a given IP or a session with no rudp port set yet
		if (
			(curr = comListFindProtocolId(CAPI_RUDP_PROTOCOL, CAPI_ntohl(id)))
		){
			// query stuff
			curr->lastsenderid = id;
			curr->lastsender = ((struct sockaddr_in*)&in_addr)->sin_addr.s_addr;
			curr->lastsenderport = ((struct sockaddr_in*)&in_addr)->sin_port;
			if (add_to_receive_queue(curr, ptr, size)){
				got_packets++;
			}
		}

		leave_cs();
		// check if we were able to dispatch it.
		// assert (curr);
	}
	
	return got_packets;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* receive data from a comms session */
int ComRUDPGet(ComAPIHandle c)
{
	if(c)
	{
		ComIP
			*cudp;
		
		int
			did_send = 0,
			needed,
			msg_size=0;
		
		long
			now;
		
		Reliable_Packet
			*lp,	// last packet
			*cp,	// current packet
			*np;	// next packet
		
		cudp = (ComIP *)c;
		
		now = GetTickCount();

		// Check if we should send the reset stuff

		if ((cudp->rudp_data.reset_send != RUDP_WORKING) && (now - cudp->rudp_data.last_send_time > RUDP_RESEND_TIME))
		{
			if (send_rudp_packet (cudp, NULL, 0)){
				did_send = 1;
			}
		}

		// Get all packets from this handle.
		// This will copy the packets into our receive queue
		get_rudp_packets (cudp);
		
		if (cudp->rudp_data.reset_send != RUDP_WORKING)
		{
			if (did_send)
			{
				return -2;
			}
			else
			{
				return 0;
			}
		}

		// Ping is important
		if (now - cudp->rudp_data.last_ping_send_time > RUDP_PING_TIME)
		{
			cudp->rudp_data.reset_send = RUDP_PING;
			send_rudp_packet (cudp, NULL, 0);
			cudp->rudp_data.reset_send = RUDP_WORKING;
		}

		// Check for completed messages
		cp = cudp->rudp_data.receiving;
		lp = NULL;
		while (cp)
		{
			// Find the last received packet that is sequential, so 
			// we can do the ack back to the other side
			if ((cp) && ((cp->sequence_number & 0xffff) == ((cudp->rudp_data.last_received + 1) & 0xffff)))
			{
				cudp->rudp_data.last_received = cp->sequence_number;
			}

			if ((cp->sequence_number & 0xffff) == ((cudp->rudp_data.last_dispatched + 1) & 0xffff))
			{
				// If it's the first part of a packetized message, try to build
				// the message and copy into the receive buffer
				if ((!msg_size) && (cp->message_slot == 0) && (!cp->dispatched))
				{
					needed = cp->message_parts - 1;
					np = cp->next;

					//while (np && needed) // JB 010223 CTD
					while (np && needed && !F4IsBadReadPtrC(np, sizeof(Reliable_Packet))) // JB 010223 CTD
					{
						if (np->message_number == cp->message_number){
							needed--;
						}

						np = np->next;
					}

					if (!needed){
						// We've got the entire message, copy it into the receive buffer
						np = cp;
						needed = cp->message_parts;

						while (np && needed){
							if (np->message_number == cp->message_number){
								int offset = np->message_slot * (cudp->ideal_packet_size - MAX_RUDP_HEADER_SIZE);
								memcpy(cudp->recv_buffer.buf+offset, np->data, np->size);
								np->dispatched = 1;
								free(np->data);
								np->data = NULL;
								msg_size += np->size;
								needed--;

//								MonoPrint ("Dispatching %d\n", np->sequence_number); 

								cudp->rudp_data.last_dispatched = np->sequence_number;
							}

							np = np->next;
						}
					}
				}
			}
			
			// if it's been dispatched, and it's less that our sequence number, 
			// then remove this entry from the receive queue, we no longer 
			// care about it.
			if ((cp->dispatched) && ((cp->sequence_number - (cudp->rudp_data.last_received + 1)) & 0x8000))
			{
				if (lp)
				{
					lp->next = cp->next;
				}
				else
				{
					cudp->rudp_data.receiving = cp->next;
				}

				if (cp->data)
				{
					free(cp->data);
				}

				free(cp);

				if (lp)
				{
					cp = lp->next;
				}
				else
				{
					cp = cudp->rudp_data.receiving;
				}
			}
			else
			{
				lp = cp;
				cp = cp->next;
			}
		}
		
		// Check for completed oob messages
		cp = cudp->rudp_data.oob_receiving;
		lp = NULL;
		while (cp)
		{
			// Find the last received packet that is sequential, so 
			// we can do the ack back to the other side
			if ((cp) && ((cp->sequence_number & 0xffff) == ((cudp->rudp_data.last_oob_received + 1) & 0xffff)))
			{
				cudp->rudp_data.last_oob_received = cp->sequence_number;
			}

			if ((cp->sequence_number & 0xffff) == ((cudp->rudp_data.last_oob_dispatched + 1) & 0xffff))
			{
				// If it's the first part of a packetized message, try to build
				// the message and copy into the receive buffer
				if ((!msg_size) && (cp->message_slot == 0) && (!cp->dispatched))
				{
					needed = cp->message_parts - 1;
					np = cp->next;

					while (np && needed)
					{
						if (np->message_number == cp->message_number)
						{
							needed--;
						}

						np = np->next;
					}

					if (!needed)
					{
						// We've got the entire message, copy it into the receive buffer
						np = cp;
						needed = cp->message_parts;

						while (np && needed)
						{
							if (np->message_number == cp->message_number)
							{
								int offset = np->message_slot * (cudp->ideal_packet_size - MAX_RUDP_HEADER_SIZE);
								memcpy(cudp->recv_buffer.buf+offset, np->data, np->size);
								np->dispatched = 1;
								free(np->data);
								np->data = NULL;
								msg_size += np->size;
								needed--;

//								MonoPrint ("Dispatching OOB %d\n", np->sequence_number); 

								cudp->rudp_data.last_oob_dispatched = np->sequence_number;
							}

							np = np->next;
						}
					}
				}
			}
			
			// if it's been dispatched, and it's less that our sequence number, 
			// then remove this entry from the receive queue, we no longer 
			// care about it.
			if ((cp->dispatched) && ((cp->sequence_number - (cudp->rudp_data.last_oob_received + 1)) & 0x8000))
			{
				if (lp)
				{
					lp->next = cp->next;
				}
				else
				{
					cudp->rudp_data.oob_receiving = cp->next;
				}

				if (cp->data)
				{
					free(cp->data);
				}

				free(cp);

				if (lp)
				{
					cp = lp->next;
				}
				else
				{
					cp = cudp->rudp_data.oob_receiving;
				}
			}
			else
			{
				lp = cp;
				cp = cp->next;
			}
		}
		
		// Clean up all packets that we know got through from the sending queue..
		cp = cudp->rudp_data.sending;
		lp = NULL;

		while (cp)
		{
			if ((cp->sequence_number - (cudp->rudp_data.last_sequence + 1)) & 0x8000)
			{
				// this packet got through
				// since its sequence number is less than last_sequence number
				// the other guy has received

				// we now accept out of sequence packets in sending queue

				// assert(cudp->rudp_data.sending == cp);
				
				if (lp)
				{
					lp->next = cp->next;
				}
				else
				{
					cudp->rudp_data.sending = cp->next;
				}

				if (cp == cudp->rudp_data.last_sent)
				{
					cudp->rudp_data.last_sent = lp;
				}
				
				if (cp->data)
				{
					free(cp->data);
				}

				lp = NULL;
				cp = cudp->rudp_data.sending;
			}
			else
			{
				lp = cp;
				cp = cp->next;
			}
		}
		
		// Clean up all packets that we know got through from the oob sending queue..
		cp = cudp->rudp_data.oob_sending;
		lp = NULL;

		while (cp)
		{
			if ((cp->sequence_number - (cudp->rudp_data.last_oob_sequence + 1)) & 0x8000)
			{
				// this packet got through
				// since its sequence number is less than last_sequence number
				// the other guy has received

				// we now accept out of sequence packets in sending queue

				// assert(cudp->rudp_data.sending == cp);
				
				if (lp)
				{
					lp->next = cp->next;
				}
				else
				{
					cudp->rudp_data.oob_sending = cp->next;
				}

				if (cp == cudp->rudp_data.oob_last_sent)
				{
					cudp->rudp_data.oob_last_sent = lp;
				}
				
				if (cp->data)
				{
					free(cp->data);
				}

				lp = NULL;
				cp = cudp->rudp_data.oob_sending;
			}
			else
			{
				lp = cp;
				cp = cp->next;
			}
		}
		
		// Check if we should send an ack
		// If we've not send them our last_received, or one second timeout for ack
		if
		(
			(cudp->rudp_data.last_sent_received != cudp->rudp_data.last_received) &&
			(now - cudp->rudp_data.last_send_time > RUDP_ACK_WAIT_TIME)
		)
		{
			cudp->rudp_data.last_sent_received = -1;
		}

		if
		(
			(cudp->rudp_data.last_oob_sent_received != cudp->rudp_data.last_oob_received) &&
			(now - cudp->rudp_data.last_oob_send_time > RUDP_OOB_ACK_WAIT_TIME)
		)
		{
			cudp->rudp_data.last_oob_sent_received = -1;
		}

		if ((cudp->rudp_data.last_sent_received == -1) || (cudp->rudp_data.last_oob_sent_received == -1))
		{
			if (send_rudp_packet (cudp, NULL, 0))
			{
				did_send = 1;
			}
		}

		if ((cudp->rudp_data.send_ack) && (now - cudp->rudp_data.last_send_time > RUDP_ACK_WAIT_TIME))
		{
			cudp->rudp_data.send_ack = FALSE;
			if (send_rudp_packet (cudp, NULL, 0))
			{
				did_send = 1;
			}
		}
		
		if ((cudp->rudp_data.send_oob_ack) && (now - cudp->rudp_data.last_oob_send_time > RUDP_ACK_WAIT_TIME))
		{
			cudp->rudp_data.send_oob_ack = FALSE;
			if (send_rudp_packet (cudp, NULL, 0))
			{
				did_send = 1;
			}
		}

		// Return the size of the message we got (if any)
		if (msg_size)
		{
			return msg_size;
		}
		else if (did_send)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

unsigned long ComRUDPQuery(ComAPIHandle c, int querytype)
{
	if(c)
	{
		ComIP
			*cudp;

		cudp = GETActiveCOMHandle(c);  

		switch(querytype)
		{
			case COMAPI_MESSAGECOUNT:
				return cudp->sendmessagecount + ((ComIP *)c)->recvmessagecount;
				break;
			case COMAPI_RECV_MESSAGECOUNT:
				return ((ComIP *)c)->recvmessagecount;
				break;
			case COMAPI_SEND_MESSAGECOUNT:
				return cudp->sendmessagecount;
				break;
			case COMAPI_RECV_WOULDBLOCKCOUNT:
				return ((ComIP *)c)->recvwouldblockcount;
				break;
			case COMAPI_SEND_WOULDBLOCKCOUNT:
				return cudp->sendwouldblockcount;
				break;
			case COMAPI_RECEIVE_SOCKET:
				return ((ComIP *)c)->recv_sock;
				break;
			case COMAPI_SEND_SOCKET:
				return ((ComIP *)c)->send_sock;
				break;
			case COMAPI_RELIABLE:
				return 0;
				break;
			case COMAPI_RUDP_CACHE_SIZE:
				return get_rudp_max_queue_length ();
				break;
			case COMAPI_SENDER:
				// We always return from the correct IP address - given packet queues.
				//return CAPI_ntohl(((ComIP *)c)->address.sin_addr.s_addr);
				//sfr: using lastsender like udp
				return CAPI_ntohl(((ComIP *)c)->lastsender);
				break;
			// sfr: converts
			// port info
			case COMAPI_SENDER_PORT:
				return (long)(CAPI_ntohs((short)((ComIP*)c)->lastsenderport));
				break;
			// sfr: id of sender
			case COMAPI_ID:
				return (CAPI_ntohl(((ComIP*)c)->lastsenderid));
				break;
			case COMAPI_CONNECTION_ADDRESS:
				return CAPI_ntohl(((ComIP *)c)->sendAddress.sin_addr.s_addr);
				break;
			case COMAPI_MAX_BUFFER_SIZE:
				//          return ((ComIP *)c)->max_buffer_size - sizeof(ComAPIHeader);
				return 0;
				break;
			case COMAPI_ACTUAL_BUFFER_SIZE:
				return ((ComIP *)c)->buffer_size - sizeof(ComAPIHeader);
				break;
				
			case COMAPI_PROTOCOL:
				return  c->protocol;
				break;
			case COMAPI_STATE:
				return  COMAPI_STATE_CONNECTED;
				break;
			case COMAPI_RUDP_HEADER_OVERHEAD:
				return MAX_RUDP_HEADER_SIZE;
				break;

			case COMAPI_PING_TIME:
			{
				if (((ComIP *)c)->rudp_data.reset_send == RUDP_EXIT){
					return (unsigned long) -1;
				}
				else if (((ComIP *)c)->rudp_data.reset_send == RUDP_DROP){
					return (unsigned long) -2;
				}
				else {
					int diff;
					diff = GetTickCount () - ((ComIP *)c)->rudp_data.last_ping_recv_time;
					return diff;
				}
			}
			case COMAPI_BYTES_PENDING:
			{
				Reliable_Packet
					*rp;	// reliable packet

				int
					now,
					time,
					size;

				size = 0;
				now = GetTickCount ();

				if (cudp->rudp_data.last_ping_recv_time - now > 4 * RUDP_PING_TIME){
					return 0;
				}

				rp = cudp->rudp_data.sending;
				while (rp && !F4IsBadReadPtrC(rp, sizeof(Reliable_Packet))) // JB 010619 CTD
				{
					time = RUDP_RESEND_TIME * 2;

					if ((!rp->acknowledged) && ((int)(now - rp->last_sent_at) > time)){
						size += rp->size;
					}

					rp = rp->next;
				}

				rp = cudp->rudp_data.oob_sending;
				while (rp && !F4IsBadReadPtrC(rp, sizeof(Reliable_Packet))) // JB 010619 CTD
				{
					time = RUDP_RESEND_TIME * 2;

					if ((!rp->acknowledged) && ((int)(now - rp->last_sent_at) > time)){
						size += rp->size;
					}

					rp = rp->next;
				}

				return size;
			}
				
			default:
				return 0;
				
		}
	}
	return 0;
	
	
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

unsigned long ComRUDPGetTimeStamp(ComAPIHandle c)
{
	if(c)
	{
		ComIP *cudp= (ComIP *)c;
		
		return cudp->timestamp;
	}
	else
	{
		return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* end a comms session */
// always called within CS
void ComRUDPClose(ComAPIHandle c)
{
	int count;
	int sockerror;
	CAPIList *curr = 0;
	
//	MonoPrint ("Closing RUDP\n");

	if(c)
	{
		ComIP *cudp = (ComIP *)c;
//		int trueValue = 1;
//		int falseValue = 0;

		if ((cudp->rudp_data.sequence_number) || (cudp->rudp_data.oob_sequence_number))
		{
			cudp->rudp_data.reset_send = RUDP_EXIT;
		}
		else
		{
			cudp->rudp_data.reset_send = RUDP_DROP;
		}
			
		for (count = 0; count < 8; count ++)
		{
			send_rudp_packet (cudp, NULL, 0);
			send_rudp_packet (cudp, NULL, 0);
			send_rudp_packet (cudp, NULL, 0);
			send_rudp_packet (cudp, NULL, 0);
			Sleep (100);
		}

		if (cudp->parent)
		{
			ComIP *parent;
			parent = cudp->parent;
			//GlobalListHead = CAPIListRemove(GlobalListHead,c);
			comListRemove(cudp);

			
			if (cudp->send_buffer.buf)
			{
				free ( cudp->send_buffer.buf);
			}

			free(cudp);
			parent->referencecount--;
			cudp = parent;
		}
		else 
		{
			cudp->referencecount--;
		}
		
		if(cudp->referencecount)
		{	
			return;
		}
		
		//GlobalListHead = CAPIListRemove(GlobalListHead,(ComAPIHandle)cudp);
		comListRemove(cudp);
		
		
		if (sockerror = CAPI_closesocket(cudp->recv_sock))
		{
			sockerror = CAPI_WSAGetLastError();
			switch(sockerror)
			{
				case WSANOTINITIALISED:
					break;
				case WSAENETDOWN:
					break;
				case WSAENOTSOCK:
					break;
				case WSAEINPROGRESS:
					break;
				case WSAEINTR:
					break;
				case WSAEWOULDBLOCK:
					break;
				default :
					break;
			}
		}
		
		if (sockerror = CAPI_closesocket(cudp->send_sock))
		{
			sockerror = CAPI_WSAGetLastError();
			switch(sockerror)
			{
				case WSANOTINITIALISED:
					break;
				case WSAENETDOWN:
					break;
				case WSAENOTSOCK:
					break;
				case WSAEINPROGRESS:
					break;
				case WSAEINTR:
					break;
				case WSAEWOULDBLOCK:
					break;
				default :
					break;
			}
		}

		WS2Connections--;
		
		/* if No more connections then WSACleanup() */
		if (!WS2Connections)
		{
			if(sockerror = CAPI_WSACleanup())
			{
				sockerror = CAPI_WSAGetLastError();
			}

#ifdef LOAD_DLLS
			FreeLibrary(hWinSockDLL);
			hWinSockDLL = 0;
#endif
		}
		
		if(cudp->recv_buffer.buf)
		{
			free(cudp->recv_buffer.buf);
		}

		if(cudp->send_buffer.buf)
		{
			free(cudp->send_buffer.buf);
		}

		if(cudp->rudp_data.real_send_buffer)
		{
			free(cudp->rudp_data.real_send_buffer);
		}

		if(cudp->compression_buffer)
		{
			free(cudp->compression_buffer);
		}

		// JB 010718 remove the protocol test?
		if (c->protocol >=0 && c->protocol <= CAPI_LAST_PROTOCOL &&// JB 010222 CTD
			!F4IsBadReadPtrC(cudp, sizeof(ComIP))) // JB 010710 CTD
		{
			free(cudp);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int comms_compress (char *in, char *out, int size)
{
	char
		*current,
		*ptr;

	int
		best,
		best_size,
		run,
		loop,
		index,
		newsize;

	current = out;
	*current = 0x00;
	out ++;
	newsize = 1;

	index = 0;

	ptr = in;

	while (index < size)
	{
		best = 0;
		best_size = 0;

		for (loop = max (0, index - 127); loop < index; loop ++)
		{
			if (in[loop] == *ptr)
			{
				for (run = 1; (run < 127) && (loop + run < index) && (index + run < size); run ++)
				{
					if (in[loop + run] == ptr[run])
					{
						if (run + 1 > best_size)
						{
							best_size = run + 1;
							best = loop;
						}
					}
					else
					{
						break;
					}
				}
			}
		}

		if (best_size <= 2)
		{
			*out = *ptr;
			out ++;
			ptr ++;
			(*current) ++;
			newsize ++;

			if (*current == 0x78)
			{
				current = out;
				*current = 0x00;
				out ++;
				newsize ++;
			}

			index ++;
		}
		else
		{
			if (*current == 0x00)
			{
				out --;
				newsize --;
			}

			*out = (char)(0x80 | best_size);
			out ++;
			*out = (char)(index - best);
			out ++;
			newsize += 2;

			current = out;
			*current = 0x00;
			out ++;
			newsize ++;

			index += best_size;
			ptr += best_size;
		}
	}

	if (*current == 0x00)
	{
		newsize --;
	}

	return newsize;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int comms_decompress (char *in, char *out, int size)
{
	char
		*offset;
	int
		len,
		index,
		newsize;

	newsize = 0;

	while (newsize < size)
	{
		len = *in;
		in ++;

		if (len & 0x80)
		{
			len = len & 0x7f;
			index = *(unsigned char*) in;
			in ++;
			offset = &out[-index];

			newsize += len;

			while (len)
			{
				len --;
				*out = *offset;
				out ++;
				offset ++;
			}
		}
		else
		{
			newsize += len;

			while (len)
			{
				len --;
				*out = *in;
				out ++;
				in ++;
			}
		}
	}

	return newsize;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
