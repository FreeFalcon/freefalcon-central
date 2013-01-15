#if WIN32
#pragma optimize( "", off ) // JB 010718
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "capiopt.h"
#include "capi.h"
#include "capipriv.h"
#include "ws2init.h"
#include "wsprotos.h"
#include "tcp.h"
#include "debuggr.h"
// sfr: bandwidth
#include "capibwcontrol.h"

// sfr: for new isbad checks
#include "falclib/include/isbad.h"

// my address information
unsigned short myReliableRecvPort = 0;
unsigned short myRecvPort = 0;

static struct sockaddr_in comBroadcastAddr, comRecvAddr;

extern int ComDPLAYEnumProtocols(int *protocols, int maxprotocols);
DWProc_t CAPI_TimeStamp = NULL;

unsigned long ComAPILastError = 0;

static void (*info_callback) (ComAPIHandle c, int send, int msgsize) = NULL;


// JB 010718 start

static int init_cs = FALSE;
static CRITICAL_SECTION cs;
void enter_cs (void){
	if (!init_cs){
		InitializeCriticalSection (&cs);
		init_cs = TRUE;
	}

	EnterCriticalSection (&cs);
}

void leave_cs (void){
	if (init_cs){
		LeaveCriticalSection (&cs);
	}
}
// JB 010718 end


void ComAPIClose(ComAPIHandle c)
{
	enter_cs (); // JB 010718

	if (c) {
		(*c->close_func)(c);
	}

	leave_cs (); // JB 010718
}

int ComAPISend(ComAPIHandle c, int msgsize, int type){
	int rc = 0;
	int isBad;
	enter_cs ();
	
	// sfr: another hack by JB...
	isBad = F4IsBadReadPtrC(c, sizeof(ComAPI)) || F4IsBadCodePtrC((FARPROC)(*c->send_func));
	if (c && !isBad) {
		if (info_callback){
			info_callback (c, 1, msgsize);
		}
		
		if (c->send_func){
			rc = (*c->send_func)(c, msgsize, FALSE, type);
		}
	}
	leave_cs ();

	return rc;
}


int ComAPISendDummy(ComAPIHandle c, unsigned long ip, unsigned short port){
	int rc = 0;
	enter_cs ();
	
	if (c != NULL){
		if (c->send_dummy_func){
			rc = (*c->send_dummy_func)(c, ip, port);
		}
	}
	leave_cs ();

	return rc;

}

int ComAPISendOOB(ComAPIHandle c, int msgsize, int type){
	int rc = 0;
	enter_cs (); // JB 010718
	if(c){
		if (info_callback){
			info_callback (c, 1, msgsize);
		}
		
		if (c->send_func){ // JB 010718
			rc = (*c->send_func)(c, msgsize, TRUE, type);
		}
	}
	leave_cs ();
	return rc;
}

/*int ComAPISendX(ComAPIHandle c, int msgsize,ComAPIHandle Xcom){
	int rc = 0;
	enter_cs ();
	if(c){
		if (info_callback){
			info_callback (c, 2, msgsize);
		}
		
		rc = (*c->sendX_func)(c, msgsize,Xcom);
		
		if (rc > 0){
			use_bandwidth(rc, 0);
		}
	}
	leave_cs ();	
	return rc;
}*/

/* recive data from a comms session */

int ComAPIGet(ComAPIHandle c)
{
	int size = 0;
	int isBad;
	
	enter_cs ();
	
	// sfr: another hack by JB...
	isBad = c && (F4IsBadReadPtrC(c, sizeof(ComAPI)) || F4IsBadCodePtrC((FARPROC)(*c->recv_func)));
	if(c && !isBad){ // JB 010404 CTD
		size = (*c->recv_func)(c);
		if (info_callback){
			info_callback (c, 0, size);
		}		
	}

	leave_cs ();
	
	return size;
}

/* set the group to send and recieve data from */

void ComAPIGroupSet(ComAPIHandle c, int group)
{
	c;
	group;
}

/* get the local hosts unique id len */

int ComAPIHostIDLen(ComAPIHandle c)
{
	if(c) {
		return 4;
    }
	else {
		return 0;
    }
}

/* get the local hosts unique id */

int ComAPIHostIDGet(ComAPIHandle c, char *buf, int reset)
{
	int ret_val = 0;

	enter_cs ();
	
	if(c) {
		ret_val = (*c->addr_func)(c, buf, reset);
	}

	leave_cs ();
	
	return ret_val;
}

/* get the associated write buffer */

char *ComAPISendBufferGet(ComAPIHandle c)
{
	char *ret_val = 0;
	enter_cs ();

	if (c) {
		ret_val = (*c->send_buf_func)(c);
	}
	leave_cs ();

	return ret_val;
}

char *ComAPIRecvBufferGet(ComAPIHandle c)
{
	char *ret_val = 0;
	int isBad;
	enter_cs ();

	// sfr: another hack by JB...
	isBad = c && (F4IsBadReadPtrC(c, sizeof(ComAPI)) || F4IsBadCodePtrC((FARPROC)(*c->recv_buf_func)));

	if (c && !isBad){
		ret_val = (*c->recv_buf_func)(c);
    }
	leave_cs ();

	return ret_val;
}

/* get the current send+receive message count */

unsigned long ComAPIQuery(ComAPIHandle c, int querytype)
{
	unsigned long
		ret_val = 0;
	enter_cs ();

	if(c){
		ret_val = (*c->query_func)(c,querytype);
	}
	else {
		switch (querytype){
			case COMAPI_TCP_HEADER_OVERHEAD:
			{
				ret_val = sizeof(tcpHeader) + 40;	// Size of underlying header.
				break;
			}

			case COMAPI_UDP_HEADER_OVERHEAD:
			{
				ret_val = sizeof(ComAPIHeader);
				break;
			}

			case COMAPI_RUDP_HEADER_OVERHEAD:
			{
				ret_val = MAX_RUDP_HEADER_SIZE;
				break;
			}
		}
	}
	leave_cs ();
	return ret_val;
}

#if 0
int ComAPIEnumProtocols(int *protocols, int maxprotocols)
{
//	int i=0,size=0, numprotos=0;
	WSADATA wsaData;
	int ourprotos=0;
	
	if (InitWS2(&wsaData) == 0)
	{
		return 0;
	}
		
	if(maxprotocols >= 2)
	{
		*protocols = CAPI_TCP_PROTOCOL;
		ourprotos++;
		protocols++;
		*protocols = CAPI_UDP_PROTOCOL;
		ourprotos++;
	}
	
	/* Now use DPLAY to enumerate it's avaliable protocols */
	ourprotos += ComDPLAYEnumProtocols(protocols, maxprotocols - ourprotos);

	return ourprotos;
}
#endif

/* convert host ip address to string */

char *ComAPIinet_htoa(u_long ip)
{
	struct in_addr addr;
	
	if(CAPI_htonl == NULL)
	{
		return NULL;
	}

	addr.s_addr = CAPI_htonl(ip);
	
	if(CAPI_inet_ntoa == NULL)
	{
		return NULL;
	}

	return CAPI_inet_ntoa(addr);
}

/* convert net ipaddress to string */

char *ComAPIinet_ntoa(u_long ip)
{
	struct in_addr addr;
	
	addr.s_addr = ip;
	
	if(CAPI_inet_ntoa == NULL)
	{
		return NULL;
	}

	return CAPI_inet_ntoa(addr);
}

/* convert net ipaddress to string */

unsigned long ComAPIinet_haddr(char * IPAddress)
{
	unsigned long ipaddress;
	
	if(CAPI_inet_addr == NULL)
	{
		return 0;
	}

	ipaddress = CAPI_inet_addr(IPAddress);
	
	if(CAPI_ntohl == NULL)
	{
		return 0;
	}

	return CAPI_ntohl(ipaddress);
}

unsigned long ComAPIGetLastError(void)
{
	return ComAPILastError;
}

void ComAPISetTimeStampFunction(unsigned long (*TimeStamp)(void))
{
	CAPI_TimeStamp = TimeStamp;
    CAPI_TimeStamp ();
}

unsigned long ComAPIGetTimeStamp(ComAPIHandle c)
{
	unsigned long
		ret_val = 0;
	enter_cs ();
	if (c){
		ret_val = (c->get_timestamp_func)(c);
    }
	leave_cs ();
	return ret_val;
}

void ComAPIRegisterInfoCallback (void (*func)(ComAPIHandle, int, int))
{
	info_callback = func;
}

/////////////////////
// Bandwidth stuff //
/////////////////////

void ComAPIBWStart(){
	start_bandwidth();
}

void ComAPIBWPlayerJoined(){
	player_joined();
}

void ComAPIBWPlayerLeft(){
	player_left();
}

void ComAPIBWEnterState(int state){
	enter_state((bwstates)state);
}

//int ComAPIBWGetStatus(int isReliable){
//	return get_status(isReliable);
//}
///////////////////
// End Bandwidth //
///////////////////

int ComAPIInitComms(void)
{
	WSADATA wsaData;
	int ret=1;
	
	if(!WS2Connections) {
		ret = InitWS2(&wsaData);
		WS2Connections--;

		/* if No more connections then WSACleanup() */
		if (!WS2Connections) {
            CAPI_WSACleanup();
		}
	}

	return ret;
}

void ComAPISetName (ComAPIHandle c, char *name_in)
{
	if (c->name)
	{
		free(c->name);
	}
	c->name = (char*)malloc (strlen (name_in) + 1);
	strcpy (c->name, name_in);
}

void ComAPISetLocalPorts(unsigned short b, unsigned short r){
	myRecvPort =  CAPI_htons(b);
	myReliableRecvPort = CAPI_htons(r);
}

unsigned short ComAPIGetRecvPort(ComAPIHandle c){
	// this is the same for all coms of same type
	return CAPI_ntohs( ((ComIP*)c)->recAddress.sin_port );
}

unsigned short ComAPIGetPeerRecvPort(ComAPIHandle c){
	// we send to this address, so its his receive port
	return CAPI_ntohs( ((ComIP*)c)->sendAddress.sin_port );
}

unsigned long ComAPIGetPeerIP(ComAPIHandle c){
	// we send to this address, so this is his IP
	return CAPI_ntohl( ((ComIP*)c)->sendAddress.sin_addr.S_un.S_addr );
}

unsigned long ComAPIGetPeerId(ComAPIHandle c){
	return CAPI_ntohl( ((ComIP*)c)->id );
}

int ComAPIGetProtocol(ComAPIHandle c){
	return c->protocol;
}

unsigned short ComAPIGetMyRecvPort(){
	return CAPI_ntohs(myRecvPort);
}

unsigned short ComAPIGetMyReliableRecvPort(){
	return CAPI_ntohs(myReliableRecvPort);
}

void ComAPISetMyRecvPort(unsigned short port){
	myRecvPort = CAPI_htons(port);
}

void ComAPISetMyReliableRecvPort(unsigned short port){
	myReliableRecvPort = CAPI_htons(port);
}

int ComAPIPrivateIP(unsigned long ip){
	// ip is composed 
	// XXX.YYY.ZZZ.WWW
	unsigned long x, y, z, w; //use long to avoid warning
	x = (ip & 0xFF000000) >> 24;
	y = (ip & 0x00FF0000) >> 16;
	z = (ip & 0x0000FF00) >> 8;
	w = (ip & 0x000000FF);
	if (
		((x == 127) && (y == 0) && (z == 0) && (w == 1)) || // localhost
		(x == 10) || // class A reserved
		((x == 172) && ((y >= 16 ) && (y < 31))) || // class B reserver
		((x == 192) && (y == 168) && ((z >= 0) && (z < 255))) // class C reserved
	){
		return 1;
	}
	else {
		return 0;
	}
}

long ComAPIGetIP(const char *address){
	struct hostent *h = CAPI_gethostbyname(address);
	if ((h == NULL) || (h->h_addr_list == NULL)){
		return 0;
	}
	else {
		return CAPI_ntohl(*((long*)h->h_addr_list[0]));
		
	}
}

