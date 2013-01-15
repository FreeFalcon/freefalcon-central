#pragma optimize( "", off ) // JB 010718

#include <stdio.h>
#include <stdlib.h>

#include <winsock.h>
#include <assert.h>
#include <time.h>
#include "capiopt.h"
#include "capi.h"
#include "capipriv.h"
#include "ws2init.h"
#include "wsprotos.h"
//sfr: new includes
#include "udp.h"
#include "ComList.h"
#include "capibwcontrol.h"

// sfr: for new isbad checks
#include "falclib/include/isbad.h"

extern int ComAPILastError;
int ComIPGetHostIDIndex = 0;
int force_ip_address = 0;
extern DWProc_t CAPI_TimeStamp;

void cut_bandwidth (void);

extern int MonoPrint (char *, ...);


int comms_compress (char *in, char *out, int size);
int comms_decompress (char *in, char *out, int size);

/* List head for connection list */
//extern CAPIList *GlobalListHead;
extern HANDLE    GlobalListLock;


/* Mutex macros */
#define SAY_ON(a)            
#define SAY_OFF(a)			 
#define CREATE_LOCK(a,b)                { a = CreateMutex( NULL, FALSE, b ); if( !a ) DebugBreak(); }
#define REQUEST_LOCK(a)                 { int w = WaitForSingleObject(a, INFINITE); {SAY_ON(a);} if( w == WAIT_FAILED ) DebugBreak(); }
#define RELEASE_LOCK(a)                 { {SAY_OFF(a);} if( !ReleaseMutex(a)) DebugBreak();   }
#define DESTROY_LOCK(a)                 { if( !CloseHandle(a)) DebugBreak();   }


static struct sockaddr_in comBroadcastAddr, comRecvAddr;

/* forward function declarations */
void ComUDPClose(ComAPIHandle c);
int ComUDPSend(ComAPIHandle c, int msgsize, int oob, int type);
int ComUDPSendDummy(ComAPIHandle c, unsigned long ip, unsigned short port);
int ComUDPSendX(ComAPIHandle c, int msgsize, int oob, int type, ComAPIHandle Xcom);
int ComUDPGet(ComAPIHandle c);

int ComIPHostIDGet(ComAPIHandle c, char *buf, int reset);
char *ComIPSendBufferGet(ComAPIHandle c);
char *ComIPRecvBufferGet(ComAPIHandle c);
unsigned long ComUDPQuery(ComAPIHandle c, int querytype);
unsigned long ComUDPGetTimeStamp(ComAPIHandle c);

extern void enter_cs (void);
extern void leave_cs (void);

static ComAPIHandle ComUDPOpenSendClone(
	char *name, 
	ComIP *parentCom, 
	int buffersize, 
	char *gamename, 
	int udpPort, 
	unsigned long IPaddress,
	unsigned long id
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define GETActiveCOMHandle(c)   (((ComIP *)c)->parent == NULL) ? ((ComIP *)c) : ((ComIP *)c)->parent

CAPIList * CAPIListAppend( CAPIList * list );
CAPIList * CAPIListRemove( CAPIList * list ,ComAPIHandle c);
CAPIList * CAPIListAppendTail( CAPIList * list );

/* begin a comms session */
/* IPaddress == 0 -> Broadcast */
/* IPaddress == -1 -> Recv socket only */
ComAPIHandle ComUDPOpen(
	char *name_in,
	int buffersize, 
	char *gamename,
	unsigned short localUdpPort,
	unsigned short remoteUdpPort, 
	unsigned long IPaddress,
	unsigned long id
){
	ComIP *c;
	int err, size;
	WSADATA wsaData;
	unsigned long trueValue=1;
	enter_cs();

	if (InitWS2(&wsaData) == 0){
		leave_cs ();
		return 0;
	}

	//sfr: if we already have comms setup, use a clone to send data to new client
	c = comListFindProtocolRport(CAPI_UDP_PROTOCOL, localUdpPort);
	//if (listitem){
	if (c != NULL){
		ComAPIHandle ret_val;
		ret_val = ComUDPOpenSendClone(name_in, c, buffersize, gamename,  remoteUdpPort, IPaddress, id);
		leave_cs ();

		return ret_val;
	}

	/* add new socket connection to list */

	c = (ComIP*)malloc(sizeof(ComIP));
	memset(c,0,sizeof(ComIP));
	((ComAPIHandle)c)->name = strdup(name_in);

	/* initialize header data */
	c->apiheader.protocol = CAPI_UDP_PROTOCOL;
	c->apiheader.send_func = ComUDPSend;
	c->apiheader.send_dummy_func = ComUDPSendDummy;
	c->apiheader.sendX_func = ComUDPSendX;
	c->apiheader.recv_func = ComUDPGet;
	c->apiheader.send_buf_func = ComIPSendBufferGet;
	c->apiheader.recv_buf_func = ComIPRecvBufferGet;
	c->apiheader.addr_func = ComIPHostIDGet;
	c->apiheader.close_func = ComUDPClose;
	c->apiheader.query_func = ComUDPQuery;
	c->apiheader.get_timestamp_func = ComUDPGetTimeStamp;
	c->sendmessagecount = 0;
	c->recvmessagecount = 0;
	c->recvwouldblockcount = 0;
	c->sendwouldblockcount = 0;
	c->referencecount = 1;
	c->timestamp = 0;
	c->max_buffer_size = 0;
	c->ideal_packet_size = 0;

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

	c->compression_buffer = (char *)malloc(c->buffer_size);

	// sfr: store as network, but functions all use host order...
	// so reverse here
	ComIPHostIDGet(&c->apiheader, (char *)&c->whoami, 0);
	c->whoami = CAPI_htonl(c->whoami);

	if (IPaddress == CAPI_DANGLING_IP)
	{
		ComIPHostIDGet(&c->apiheader, (char*)&IPaddress, 0);
		IPaddress = CAPI_htonl (IPaddress);
		//IPaddress = CAPI_htonl (c->whoami);
	}

	strncpy(((ComAPIHeader *)c->send_buffer.buf)->gamename, gamename, GAME_NAME_LENGTH);
	// sfr: send id in network order
	((ComAPIHeader *)c->send_buffer.buf)->id = c->whoami;

	CAPI_ioctlsocket(c->recv_sock, FIONBIO, &trueValue);
	if(c->recv_sock == INVALID_SOCKET)
	{
		//		int error = CAPI_WSAGetLastError();
		leave_cs ();
		return 0;
	}

	/* Incoming... */
	memset ((char*)&comRecvAddr, 0, sizeof(comRecvAddr));
	comRecvAddr.sin_family       = AF_INET;
	comRecvAddr.sin_addr.s_addr  = CAPI_htonl(INADDR_ANY);
	// receive port
	comRecvAddr.sin_port         = CAPI_htons(localUdpPort);
	memcpy(&c->recAddress, &comRecvAddr, sizeof(struct sockaddr_in));

	if(err=CAPI_bind(c->recv_sock, (struct sockaddr*)&comRecvAddr,sizeof(comRecvAddr)))
	{
		leave_cs ();
		return 0;
	}

	//c->send_sock = CAPI_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	c->send_sock = c->recv_sock;

	if(c->send_sock == INVALID_SOCKET){
		leave_cs ();
		return 0;
	}

	size = CAPI_SENDBUFSIZE;
	CAPI_setsockopt (c->send_sock, SOL_SOCKET, SO_SNDBUF, (char*) &size, 4);
	size = CAPI_RECVBUFSIZE;
	CAPI_setsockopt (c->send_sock, SOL_SOCKET, SO_RCVBUF, (char*) &size, 4);

	/**  .. on ISPs modem maybe better to set to Non-Blocking on send **/
	CAPI_ioctlsocket(c->send_sock, FIONBIO, &trueValue);

	/* Outgoing... */
	memset ((char*)&c->sendAddress, 0, sizeof(c->sendAddress));
	c->sendAddress.sin_family       = AF_INET;

	if(IPaddress == 0){
		c->sendAddress.sin_addr.s_addr  = CAPI_htonl(INADDR_BROADCAST);
	}
	else {
		c->sendAddress.sin_addr.s_addr  = CAPI_htonl(IPaddress);
	}

	// send port
	c->sendAddress.sin_port         = CAPI_htons((unsigned short)remoteUdpPort);

	if(IPaddress == 0){
		c->NeedBroadcastMode = 1;
		c->BroadcastModeOn = 1;
		CAPI_setsockopt(c->send_sock, SOL_SOCKET, SO_BROADCAST, (char *)&trueValue, sizeof(int) );  
	}

	// sfr handle id
	c->id = CAPI_htonl(id);

	// sfr: add c to list
	comListAdd(c);
	leave_cs ();

	return (ComAPIHandle)c;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// JPO always called within CS
ComAPIHandle ComUDPOpenSendClone(
	char *name_in, 
	ComIP *parentCom,
	int buffersize, 
	char *gamename, 
	int udpPort, 
	unsigned long IPaddress,
	unsigned long id
){
	ComIP *c;
	CAPIList *curr = 0;


	/* add new socket connection to list */

	c = (ComIP*)malloc(sizeof(ComIP));
	memset(c,0,sizeof(ComIP));

	memcpy(c,parentCom,sizeof(ComIP));

	((ComAPIHandle)c)->name = (char*)malloc (strlen (name_in) + 1);
	strcpy (((ComAPIHandle)c)->name, name_in);

	/* initialize header data */

	c->parent = parentCom;
	parentCom->referencecount++;
	c->referencecount = 1;


	c->buffer_size =   max(parentCom->buffer_size, (int)(sizeof(ComAPIHeader) + buffersize));

	if ((c->max_buffer_size > 0) && (c->buffer_size  > c->max_buffer_size)){
		c->buffer_size = c->max_buffer_size;
	}

	c->send_buffer.buf = (char *)malloc(c->buffer_size);

	if (IPaddress == -1){
		ComIPHostIDGet(&c->apiheader, (char*)&IPaddress, 0);
		IPaddress = CAPI_htonl (IPaddress);
	}

	// header 
	strncpy(((ComAPIHeader *)c->send_buffer.buf)->gamename, gamename, GAME_NAME_LENGTH);
	// sfr: id in network order
	((ComAPIHeader *)c->send_buffer.buf)->id = c->whoami;

	/* Outgoing... */
	memset ((char*)&c->sendAddress, 0, sizeof(c->sendAddress));
	c->sendAddress.sin_family       = AF_INET;

	if(IPaddress == 0){
		c->sendAddress.sin_addr.s_addr  = CAPI_htonl(INADDR_BROADCAST);
	}
	else{
		c->sendAddress.sin_addr.s_addr  = CAPI_htonl(IPaddress);
	}

	c->sendAddress.sin_port         = CAPI_htons((unsigned short)udpPort);


	if(IPaddress == 0){
		c->NeedBroadcastMode = 1;
	}
	else {
		c->NeedBroadcastMode = 0;
	}
	//sfr id (network order)
	c->id = CAPI_htonl(id);

	comListAdd(c);
	return (ComAPIHandle)c;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* end a comms session */
/* always called from within a critical section */
void ComUDPClose(ComAPIHandle c)
{
	int sockerror;
	CAPIList
		*curr = 0;

	if(c)
	{
		ComIP *cudp = (ComIP *)c;
		//		int trueValue = 1;
		//		int falseValue = 0;

		if (cudp->parent != NULL)
		{
			ComIP *parent;
			parent = cudp->parent;
			/* remove this ComHandle from the current list */
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

#ifdef _DEBUG
		MonoPrint ("ComUDPClose Parent CH:\"%s\"\n", ((ComAPIHandle)cudp)->name);
#endif

//		GlobalListHead = CAPIListRemove(GlobalListHead,(ComAPIHandle)cudp);
		comListRemove(cudp);

		if(sockerror = CAPI_closesocket(cudp->recv_sock))
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


		if(sockerror = CAPI_closesocket(cudp->send_sock))
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
			else
			{
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

		if(cudp->send_buffer.buf){
			free(cudp->send_buffer.buf);
		}

		if(cudp->compression_buffer){
			free(cudp->compression_buffer);
		}

		// sfr: removed JB check
		//if (!F4IsBadReadPtrC(cudp, sizeof(ComIP))) // JB 010615 CTD
		free(cudp);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ComUDPSendX(ComAPIHandle c, int msgsize, int oob, int type, ComAPIHandle Xcom){
	if(c == Xcom){
		return 0;
	}
	else {
		return ComUDPSend(c, msgsize, oob, type);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* send data from a comms session */

int ComUDPSend(ComAPIHandle c, int msgsize, int oob, int type)
{
	ComIP *cudp;
	ComIP *actual;

	int senderror;
	int bytesSent;

	u_short size;
	u_short newsize;

	char buffer[1000];

	if(c == NULL){
		return 0;
	}

	cudp = (ComIP *)c;

	if(msgsize > cudp->buffer_size - (int)sizeof(ComAPIHeader)){
		return COMAPI_MESSAGE_TOO_BIG;
	}

	cudp->send_buffer.len = msgsize + sizeof(ComAPIHeader);

	/* Do we need to toggle the SO_BROADCAST state ?*/
	actual = cudp;

	if (cudp->parent != NULL){
		/* am I a SendClone ?? */
		actual   = cudp->parent;
	}

	if (F4IsBadReadPtrC(cudp, sizeof(ComIP)) || F4IsBadReadPtrC(actual, sizeof(ComIP))) // JB 010220 CTD
		return COMAPI_OUT_OF_SYNC; // JB 010220 CTD

	if(actual->BroadcastModeOn != cudp->NeedBroadcastMode)
	{
		CAPI_setsockopt(cudp->send_sock, SOL_SOCKET, SO_BROADCAST, (char *)&(cudp->NeedBroadcastMode), sizeof(int) );  
		actual->BroadcastModeOn = cudp->NeedBroadcastMode;     
	}

	size = (u_short)cudp->send_buffer.len;

	if (size < 32){
		newsize = size;
	}
	else {
		memcpy(buffer, &size, sizeof(u_short));
		memset (cudp->send_buffer.buf + size, 0, cudp->buffer_size - size);
		newsize = (unsigned short)(comms_compress (cudp->send_buffer.buf, buffer+sizeof(u_short), size));
	}

	//cudp->send_buffer.len = newsize+sizeof(u_short);

	// if not OOB, check if enough bw is available
	if (!oob){
		if (check_bandwidth(newsize + sizeof (u_short), 0, type) == 0){
			return COMAPI_WOULDBLOCK;
		}
	}

	bytesSent = 0;
	//if (docomms){
	if ((int)(newsize + sizeof (u_short)) < (int) (size))
	{
		bytesSent = CAPI_sendto
			(
				cudp->send_sock,
				buffer,
				newsize + sizeof (u_short),
				0,
				(struct sockaddr *)&cudp->sendAddress,   
				sizeof(cudp->sendAddress)
			);
	}
	else
	{
		bytesSent = CAPI_sendto
			(
				cudp->send_sock,
				cudp->send_buffer.buf,
				cudp->send_buffer.len,
				0,
				(struct sockaddr *)&cudp->sendAddress,   
				sizeof(cudp->sendAddress)
			);
	}
	//}

	senderror = bytesSent;

	if (senderror == SOCKET_ERROR)
	{
		senderror = CAPI_WSAGetLastError();
		switch(senderror)
		{
			case WSAEWOULDBLOCK:
				{
					//				MonoPrint ("WouldBlock %d %d\n", cudp->send_buffer.len, get_bandwidth_available ());
					cudp->sendwouldblockcount++;
					cut_bandwidth ();
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



#ifdef checkbandwidth

	if (test < 20 && oob == 2) 
	{
		MonoPrint("posupd size %d",bytesSent);
		test ++;
	}

	totalbwused += bytesSent;
	if (oob) oobbwused +=bytesSent;
	if (oob == 2) Posupdbwused +=bytesSent;
	if (now > laststatcound + 1000)
	{
		/*	MonoPrint("UDP Bytes pr sec %d, OOB %d, Posupds %d", 
				(int)(totalbwused*1000/(now-laststatcound)),
				(int)(oobbwused *1000/(now-laststatcound)),
				(int)(Posupdbwused*1000/(now-laststatcound)));*/
		laststatcound = now;
		totalbwused = 0;
		oobbwused = 0;
		Posupdbwused = 0;
	}

#endif		
	use_bandwidth(bytesSent, 0, type);
	cudp->rudp_data.last_ping_send_time = GetTickCount ();
	cudp->sendmessagecount++;

	return msgsize;
}

int ComUDPSendDummy(ComAPIHandle c, unsigned long ip, unsigned short port){
	int dummyBlk = 0;
	ComIP *com = (ComIP*)c;
	struct sockaddr_in to;
	memset(&to, 0, sizeof(to));
	to.sin_family = AF_INET;
	to.sin_addr.S_un.S_addr = CAPI_htonl(ip);
	to.sin_port = CAPI_htons(port);
	return CAPI_sendto(
		com->send_sock, (const void*)&dummyBlk, sizeof(int), 0, (struct sockaddr*)&to, sizeof(to)
	);
}

int ComUDPGet(ComAPIHandle c)	{

	char buffer[1000];

	while(c) {

		ComIP *cudp = (ComIP *)c;

		int recverror;
		int bytesRecvd;
		int size=0;
		struct sockaddr in_addr;

		cudp = GETActiveCOMHandle(c);
		cudp->recv_buffer.len = cudp->buffer_size;

		size =  sizeof(in_addr);
		bytesRecvd = CAPI_recvfrom(
			cudp->recv_sock,
			buffer,
			cudp->recv_buffer.len,
			0,
			&in_addr,
			&size
		);

		if (bytesRecvd == SOCKET_ERROR){
			recverror = CAPI_WSAGetLastError();

#ifdef DEBUG_COMMS
			if(recverror != WSAEWOULDBLOCK )
			{
			}
#endif

			switch(recverror)
			{
				case WSANOTINITIALISED:
					/* A successful WSAStartup must occur before using this API.*/
				case WSAENETDOWN:
					/* The network subsystem has failed. */
				case WSAEFAULT:
					/* The buf argument is not totally contained in a valid part
						 of the user address space. */
				case WSAENOTCONN:
					/* The socket is not connected. */
				case WSAEINTR:
					/* The (blocking) call was canceled via WSACancelBlockingCall. */
				case WSAEINPROGRESS:
					/* A blocking Windows Sockets 1.1 call is in progress, or
						 the service provider is still processing a callback
						 function. */
				case WSAENETRESET:
					/* The connection has been broken due to the remote host
						 resetting. */
				case WSAENOTSOCK:
					/* The descriptor is not a socket. */
				case WSAEOPNOTSUPP:
					/* MSG_OOB was specified, but the socket is not stream style
						 such as type SOCK_STREAM, out-of-band data is not supported
						 in the communication domain associated with this socket,
						 or the socket is unidirectional and supports only send
						 operations. */
				case WSAESHUTDOWN:
					/* The socket has been shutdown; it is not possible to recv
						 on a socket after shutdown has been invoked with how set
						 to SD_RECEIVE or SD_BOTH. */

				case WSAEWOULDBLOCK:
					/* The socket is marked as non-blocking and the receive
						 operation would block. */
					cudp->recvwouldblockcount++;
					return 0;

				case WSAEMSGSIZE:
					/* The message was too large to fit into the specified buffer
						 and was truncated. */
				case WSAEINVAL:
					/* The socket has not been bound with bind, or an unknown flag
						 was specified, or MSG_OOB was specified for a socket with
						 SO_OOBINLINE enabled or (for byte stream sockets only) len
						 was 0 or negative. */
				case WSAECONNABORTED:
					/* The virtual circuit was aborted due to timeout or other
						 failure. The application should close the socket as it
						 is no longer useable. */
				case WSAETIMEDOUT:
					/* The connection has been dropped because of a network
						 failure or because the peer system failed to respond. */
				case WSAECONNRESET:
					/* The virtual circuit was reset by the remote side executing
						 a "hard" or "abortive" close. The application should close
						 the socket as it is no longer useable. On a UDP datagram
						 socket this error would indicate that a previous send
						 operation resulted in an ICMP "Port Unreachable" message. */
				default :
					return 0;
			}
		}

		if (bytesRecvd > 0)
		{

			if (*(u_short*)buffer <= 700)
			{
				u_short decomp_size;

				memcpy(&decomp_size, buffer, sizeof(u_short));
				comms_decompress (buffer + sizeof(u_short), cudp->recv_buffer.buf, decomp_size);
				cudp->recv_buffer.len = decomp_size;
				bytesRecvd = decomp_size;
			}
			else
			{
				memcpy (cudp->recv_buffer.buf, buffer, bytesRecvd);
				cudp->recv_buffer.len = bytesRecvd;
			}

			// ignore messages from me
			//sfr: added port info
/*			if (!(
				(((struct sockaddr_in *)(&in_addr))->sin_addr.s_addr == cudp->whoami) &&
				(((struct sockaddr_in *)(&in_addr))->sin_port == CAPI_htons(ComAPIGetMySendPort()))
			)){*/
			if (((ComAPIHeader *)cudp->recv_buffer.buf)->id != cudp->whoami){
				// sets lastsender
				cudp->lastsender = ((struct sockaddr_in *) (&in_addr))->sin_addr.s_addr;
				cudp->lastsenderport = ((struct sockaddr_in *) (&in_addr))->sin_port;
				cudp->lastsenderid = ((ComAPIHeader *)cudp->recv_buffer.buf)->id;


				// check gamename
				if (strncmp(((ComAPIHeader *)cudp->recv_buffer.buf)->gamename, ((ComAPIHeader *)cudp->send_buffer.buf)->gamename, GAME_NAME_LENGTH) == 0)
				{
					cudp->recvmessagecount++;

					// refresh timestamp
					if (CAPI_TimeStamp) {
						cudp->timestamp = CAPI_TimeStamp();
					}
					cudp->rudp_data.last_ping_recv_time = GetTickCount ();
					//take header out
					return bytesRecvd - sizeof(ComAPIHeader);
				}
			}
		}
		else
		{
			return 0;
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// sfr: converts
// this function now gets a random ID instead of the local IP
// hopefully, this will be random enough
int ComIPHostIDGet(ComAPIHandle c, char *buf, int reset) {
	static long internalId = -1;

	// reset the ID, choose a new one
	if (reset){
		internalId = -1;
	}

	// -1 is never a valid ID
	while (internalId == -1){
		internalId = ((long)(clock()));
	}

	if (force_ip_address){
		internalId = ((long)force_ip_address);
	}

	// writes Id to buffer, host order
	memcpy(buf, &internalId, sizeof(long));

	return 0;

#if 0
	// our hostname
	char name[1024];
	// our hostent struct, with our IP
	struct hostent *hentry;
	int i;

	// we need a valid handle
	if(c == NULL)	{
		return COMAPI_HOSTID_ERROR;
	}


	// get our hostname
	if(CAPI_gethostname(name,1024) == SOCKET_ERROR)	{
		return COMAPI_HOSTID_ERROR;
	}

	// from hostname, get IP
	if ((hentry = CAPI_gethostbyname(name)) == NULL){
		return COMAPI_HOSTID_ERROR;
	}


	if (force_ip_address)	{
		buf[3] = (char)(force_ip_address & 0xff);
		buf[2] = (char)((force_ip_address >> 8)  & 0xff);
		buf[1] = (char)((force_ip_address >> 16) & 0xff);
		buf[0] = (char)((force_ip_address >> 24) & 0xff);
		return 0;
	}

	// sfr: build IP from hentry struct. 
	// shouldnt we use inet_ntoa?
	// wow this is dark magic... dont wanna get in here
	// hentry->h_addr_list[i] is an array of char
	// run all available IPs for host
	for (i=0;hentry->h_addr_list[i] != NULL; i++) {
		// we return either first IP or hostIdx IP
		// first IP
		if (i == 0) {
			*((int*)buf) = *((int*)(hentry->h_addr_list[i]));
		}
		// hostIdx IP
		if (i == ComIPGetHostIDIndex)	{
			*((int*)buf) = *((int*)(hentry->h_addr_list[i]));
			return 0;
		}
	}
	return 0;
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* get the associated write buffer */

char *ComIPSendBufferGet(ComAPIHandle c)
{
	return ((ComIP *)c)->send_buffer.buf + sizeof(ComAPIHeader);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char *ComIPRecvBufferGet(ComAPIHandle c)
{
	ComIP *cudp = (ComIP *)c;
	//	char *recvbuf=NULL;

	cudp = GETActiveCOMHandle(c) ;
	return cudp->recv_buffer.buf + sizeof(ComAPIHeader);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

unsigned long ComUDPQuery(ComAPIHandle c, int querytype)
{
	if(c)
	{
		ComIP * cudp;

		cudp = GETActiveCOMHandle(c);  

		switch(querytype)
		{
			case COMAPI_MESSAGECOUNT:
				{
					return cudp->sendmessagecount + ((ComIP *)c)->recvmessagecount;
				}

			case COMAPI_RECV_MESSAGECOUNT:
				{
					return ((ComIP *)c)->recvmessagecount;
				}

			case COMAPI_SEND_MESSAGECOUNT:
				{
					return cudp->sendmessagecount;
				}

			case COMAPI_RECV_WOULDBLOCKCOUNT:
				{
					return ((ComIP *)c)->recvwouldblockcount;
				}

			case COMAPI_SEND_WOULDBLOCKCOUNT:
				{
					return cudp->sendwouldblockcount;
				}

			case COMAPI_RECEIVE_SOCKET:
				{
					return ((ComIP *)c)->recv_sock;
				}

			case COMAPI_SEND_SOCKET:
				{
					return ((ComIP *)c)->send_sock;
				}

			case COMAPI_RELIABLE:
				{
					return 0;
				}

			case COMAPI_UDP_CACHE_SIZE:
				{
					return 0;
				}

			case COMAPI_SENDER:
				{
					return CAPI_ntohl(cudp->lastsender);
				}
			// sfr: converts
			// port info
			case COMAPI_SENDER_PORT:
				{
					return (long)(CAPI_ntohs((short)cudp->lastsenderport));
				}
			//sfr: id
			case COMAPI_ID:
				{
					return (CAPI_ntohl(cudp->lastsenderid));
				}


			case COMAPI_CONNECTION_ADDRESS:
				{
					return CAPI_ntohl(((ComIP *)c)->sendAddress.sin_addr.s_addr);
				}

			case COMAPI_MAX_BUFFER_SIZE:
				{
					return 0;
				}

			case COMAPI_ACTUAL_BUFFER_SIZE:
				{
					return ((ComIP *)c)->buffer_size - sizeof(ComAPIHeader);
				}


			case COMAPI_PROTOCOL:
				{
					return  c->protocol;
				}

			case COMAPI_STATE:
				{
					return  COMAPI_STATE_CONNECTED;
				}

			case COMAPI_UDP_HEADER_OVERHEAD:
				{
					return sizeof(ComAPIHeader);
				}

			default:
				{
					return 0;
				}
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

unsigned long ComUDPGetTimeStamp(ComAPIHandle c)
{
	if(c){
		ComIP *cudp= (ComIP *)c;
		return cudp->timestamp;
	}
	else {
		return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ComAPISetReceiveThreadPriority(ComAPIHandle c, int priority)
{
	HANDLE  threadhandle = 0;
	int     SetPriority  = 0xffffffff;

	if(c)
	{
		switch (c->protocol)
		{
			case CAPI_UDP_PROTOCOL:
				{
					ComIP *cudp= (ComIP *)c;
					threadhandle = cudp->ThreadHandle;
					break;
				}

			default:
				{
					break;
				}
		}

		switch (priority)
		{
			case CAPI_THREAD_PRIORITY_ABOVE_NORMAL:
				{
					SetPriority = THREAD_PRIORITY_ABOVE_NORMAL;
					break;
				}

			case CAPI_THREAD_PRIORITY_BELOW_NORMAL:
				{
					SetPriority = THREAD_PRIORITY_BELOW_NORMAL;
					break;
				}

			case CAPI_THREAD_PRIORITY_HIGHEST: 
				{
					SetPriority = THREAD_PRIORITY_HIGHEST;
					break;
				}

			case CAPI_THREAD_PRIORITY_IDLE: 
				{
					SetPriority = THREAD_PRIORITY_IDLE;
					break; 
				}

			case CAPI_THREAD_PRIORITY_LOWEST:
				{
					SetPriority = THREAD_PRIORITY_LOWEST;
					break;
				}

			case CAPI_THREAD_PRIORITY_NORMAL: 
				{
					SetPriority = THREAD_PRIORITY_NORMAL;
					break;
				}

			case CAPI_THREAD_PRIORITY_TIME_CRITICAL:
				{
					SetPriority = THREAD_PRIORITY_TIME_CRITICAL;
					break;
				}

			default:
				{
					break;
				}	
		}

		if (threadhandle && SetPriority != 0xFFFFFFFF)
		{
			SetThreadPriority(threadhandle,SetPriority);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//#ifdef __cplusplus
//}
//#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
