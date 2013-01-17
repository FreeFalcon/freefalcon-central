#pragma optimize( "", off ) // JB 010718

#include "capiopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <process.h>                    /* thread */
#include <winsock.h>
#include <assert.h>
#include <time.h>
#include "capi.h"
#include "capipriv.h"
#include "wsprotos.h"
#include "ws2init.h"
#include "tcp.h"
// sfr: added include
#include "capibwcontrol.h"
//#include "comdplay.h"

// sfr: for new isbad checks
#include "falclib/include/isbad.h"

extern int MonoPrint (char *, ...);

#ifdef MEM_DEBUG 
#include "heapagnt.h"
#endif

#pragma warning(disable : 4706)
#pragma warning(disable : 4127)

extern void enter_cs (void);
extern void leave_cs (void);

extern int ComAPILastError;


/* List head for connection list */
CAPIList *GlobalListHead=NULL; // any ocurrance of this outside tcp is a bug
CAPIList *GlobalGroupListHead=NULL;


/* Mutex macros */
#define SAY_ON(a)            
#define SAY_OFF(a)			 
#define CREATE_LOCK(a,b)                { a = CreateMutex( NULL, FALSE, b ); if( !a ) DebugBreak(); }
#define REQUEST_LOCK(a)                 { int w = WaitForSingleObject(a, INFINITE); {SAY_ON(a);} if( w == WAIT_FAILED ) DebugBreak(); }
#define RELEASE_LOCK(a)                 { {SAY_OFF(a);} if( !ReleaseMutex(a)) DebugBreak();   }
#define DESTROY_LOCK(a)                 { if( !CloseHandle(a)) DebugBreak();   }



 /* HEADER magic value  for TCP COM messages */
//#define     HEADER_BASE 0xFEEDFACE 
#define     HEADER_BASE 0xFACE 

/* a litle extra for the receive buffer */
#define     BUFFERPAD 0


/* some thread control flags  for the accpetconnection() thread*/
#define     THREAD_STOP         0
#define     THREAD_ACTIVE       1
#define     THREAD_TERMINATED   2
#define     SLEEP_IN_ACCEPT     1000

//#define CONNECTION_COMPLETE 0
//#define CONNECTION_PENDING  1

/* listen() backlog options */
#define     MAXBACKLOG 5


/* HandleTypes */
#define     LISTENER 0
#define     GROUP 1
#define     CONNECTION 2


extern DWProc_t CAPI_TimeStamp;

/* forward function declarations */
void          ComTCPClose(ComAPIHandle c);
int           ComTCPSend(ComAPIHandle c, int msgsize, int oob, int type);
int           ComTCPSendX(ComAPIHandle c, int msgsize, int oob, int type, ComAPIHandle Xcom);
char         *ComTCPSendBufferGet(ComAPIHandle c);
char         *ComTCPRecvBufferGet(ComAPIHandle c);
int           ComTCPGetMessage(ComAPIHandle c);
int           ComTCPRecv(ComAPIHandle c,int BytesToRecv);
static void   initComTCP(ComTCP *c);
int           ComTCPGetNbytes(ComAPIHandle c, int BytesToGet);
unsigned long ComTCPQuery(ComAPIHandle c, int querytype);
ComAPIHandle  ComTCPGetGroupHandle(int buffersize );
int           ComIPHostIDGet(ComAPIHandle c, char *buf, int reset);
char         *ComIPSendBufferGet(ComAPIHandle c);
char         *ComIPRecvBufferGet(ComAPIHandle c);
unsigned long ComTCPGetTimeStamp(ComAPIHandle c);

/* these are the thread functions to await connections */
static void          AcceptConnection(LPVOID cvoid);
static void          RequestConnection(LPVOID cvoid);


/* local functions */
static int isHeader(char *data); 
static void setHeader(char *data, int size);
static void ComTCPFreeData(ComTCP *c);

/*extern function */
//int ComDPLAYSendFromGroup(ComAPIHandle com, int msgsize, char *group_send_buffer );

/* List handler functions */
/* we keep a list of all the connections. May be useful later */

CAPIList * CAPIListAppend( CAPIList * list );
CAPIList * CAPIListRemove( CAPIList * list, ComAPIHandle com );
int        CAPIListCount( CAPIList * list);
CAPIList * CAPIListFindHandle( CAPIList * list, ComAPIHandle com );
static CAPIList * CAPIListFindTCPListenPort( CAPIList * list, short port);
void       CAPIListDestroy( CAPIList * list, void (* destructor)() );
static CAPIList * CAPIListFindTCPIPaddress(CAPIList * list, unsigned long IPaddress, unsigned short tcpPort);
static CAPIList * CAPIListFindTCPAcceptPendingExpired(CAPIList * list);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static DWORD AcceptThread;
static DWORD ConnectThread;

static int AcceptConnectioncount = 0;
static int AcceptCount = 0;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ComAPIHandle ComTCPOpenListen(int buffersize, char *gamename, int tcpPort, void (*AcceptCallback)(ComAPIHandle c, int ret) )
{
	ComTCP *c;
	int err;
//	int trueValue=1;
//	int falseValue=0;
	SOCKET listen_sock;
	CAPIList  *listitem;
	WSADATA  wsaData;

	gamename;
	
	/* InitWS2 checks that WSASstartup is done only once and increments reference count*/
	if (InitWS2(&wsaData) == 0)
	{
		return NULL;
	}
	
	enter_cs();	
	/* Is this port already being listened to?*/
	listitem = CAPIListFindTCPListenPort(GlobalListHead,(short)tcpPort);
	if (listitem)
	{
		c = (ComTCP *)listitem->com;
		REQUEST_LOCK(c->lock);
		c->referencecount++;
		RELEASE_LOCK(c->lock);

		leave_cs();
		return (ComAPIHandle)c;
	}
	
	/* CREATE NEW LISTENER */
	/* add new socket connection to list */
	/* although this is only a listener socket */
	GlobalListHead = CAPIListAppend(GlobalListHead);
	if(!GlobalListHead)
	{
	    leave_cs();
		return NULL;
	}
	
	/* allocate a new ComHandle struct */
	GlobalListHead->com = (ComAPIHandle)malloc(sizeof(ComTCP));
	c =(ComTCP*) (GlobalListHead->com);
	//GlobalListHead->ListenPort = tcpPort;
	memset(c,0,sizeof(ComTCP));
	
	/* initialize header data */
	c->accept_callback_func         = AcceptCallback;
	c->apiheader.protocol           = CAPI_TCP_PROTOCOL;
	c->apiheader.send_func          = ComTCPSend;
	c->apiheader.sendX_func         = ComTCPSendX;
	c->apiheader.recv_func          = ComTCPGetMessage;
	c->apiheader.send_buf_func      = ComTCPSendBufferGet;
	c->apiheader.recv_buf_func      = ComTCPRecvBufferGet;
	c->apiheader.addr_func          = ComIPHostIDGet;
	c->apiheader.close_func         = ComTCPClose;
	c->apiheader.query_func         = ComTCPQuery;
	c->apiheader.get_timestamp_func = ComTCPGetTimeStamp;
	c->handletype                   = LISTENER;
	c->referencecount               = 1;  
	c->buffer_size                  = sizeof(tcpHeader) + buffersize;
	c->ListenPort                   = (short)tcpPort;
	c->state                        = COMAPI_STATE_CONNECTED;
	
	/* create socket */
	listen_sock = c->recv_sock = CAPI_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if(listen_sock == INVALID_SOCKET)
	{
		ComAPILastError = err = CAPI_WSAGetLastError();
	}
	
	/* Set Server Address... */
	memset ((char*)&c->Addr, 0, sizeof(c->Addr));
	
	c->Addr.sin_family       = AF_INET;
	c->Addr.sin_addr.s_addr  = CAPI_htonl(INADDR_ANY);
	c->Addr.sin_port         = CAPI_htons((unsigned short)tcpPort);
	
	/* Bind to local address -- don't really need this but Hey ... */
	if(err=CAPI_bind(listen_sock, (struct sockaddr*)&c->Addr,sizeof(c->Addr)))
	{
//		int error = CAPI_WSAGetLastError();
	    leave_cs();
		return NULL;
	}
	
	/* now listen on this socket */
	if(err=CAPI_listen(listen_sock, MAXBACKLOG))
	{
//		int error = CAPI_WSAGetLastError();
	    leave_cs();
		return 0;
	}
	
	/* Create a mutex for this connection */
	CREATE_LOCK(c->lock,"Listen socket");
	
	/* Mark this thread ACCEPT() active in the data struct */
	c->ThreadActive = THREAD_ACTIVE;
	
	/* Start the thread which waits for socket connectiosn with accept() */	
	c->ThreadHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)AcceptConnection,(LPVOID)c,0,&AcceptThread);
	if (c->ThreadHandle == NULL)
	{
//		DWORD error = GetLastError();	   
	}
	
	SetThreadPriority(c->ThreadHandle,THREAD_PRIORITY_IDLE);
	leave_cs();
	return (ComAPIHandle)c;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ComAPIHandle ComTCPOpenConnect(int buffersize, char *gamename, int tcpPort, unsigned long IPaddress, void (*ConnectCallback)(ComAPIHandle c, int ret),int timeoutsecs )
{
	ComTCP *c;
	int err;
//	int trueValue=1;
//	int falseValue=0;
	WSADATA wsaData;
	CAPIList *listitem;
	
	gamename;

	/* InitWS2 checks that WSASstartup is done only once and increments reference count*/
	if (InitWS2(&wsaData) == 0)
	{
		return NULL;
	}
	enter_cs();	
	/* GFG */
	listitem = CAPIListFindTCPIPaddress(GlobalListHead,CAPI_htonl(IPaddress),CAPI_htons((unsigned short)tcpPort));
	if(listitem)
	{ 
		leave_cs();
		return NULL;
	}
	
	GlobalListHead = CAPIListAppend(GlobalListHead);
	if(!GlobalListHead)
	{
		leave_cs();
		return NULL;
	}
	
	/* allocate a new ComHandle struct */
	GlobalListHead->com = (ComAPIHandle)malloc(sizeof(ComTCP));
	c = (ComTCP*)(GlobalListHead->com);
	memset(c,0,sizeof(ComTCP));
	
	/* InitWS2 checks that WSASstartup is done only once and increments reference count*/
	if (InitWS2(&c->wsaData) == 0)
	{
		GlobalListHead = CAPIListRemove(GlobalListHead,(ComAPIHandle)c);
		leave_cs();
		free(c);

		return NULL;
	}
	
	leave_cs();
	
	/* initialize header data */
	c->connect_callback_func   = ConnectCallback;
	c->apiheader.protocol      = CAPI_TCP_PROTOCOL;
	c->apiheader.send_func     = ComTCPSend;
	c->apiheader.sendX_func     = ComTCPSendX;
	c->apiheader.recv_func     = ComTCPGetMessage;
	c->apiheader.send_buf_func = ComTCPSendBufferGet;
	c->apiheader.recv_buf_func = ComTCPRecvBufferGet;
	c->apiheader.query_func    = ComTCPQuery;
	c->apiheader.addr_func     = ComIPHostIDGet;
	c->apiheader.close_func    = ComTCPClose;
	c->apiheader.get_timestamp_func = ComTCPGetTimeStamp;
	
	c->buffer_size             = buffersize + sizeof(tcpHeader);
	c->send_buffer.buf         = (char *)malloc(c->buffer_size);
	c->recv_buffer.buf         = (char *)malloc( c->buffer_size);
	c->recv_buffer_start       = c->recv_buffer.buf;
	c->timeoutsecs             = (short)timeoutsecs;
	c->handletype              = CONNECTION;
	c->referencecount          = 1; 
	
	/* create socket */
	c->recv_sock = CAPI_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if(c->recv_sock == INVALID_SOCKET)
	{
		err = CAPI_WSAGetLastError();
		ComTCPFreeData(c);
		return NULL;
	}
	
	/* Server Address... */
	memset ((char*)&c->Addr, 0, sizeof(c->Addr));
	c->Addr.sin_family       = AF_INET;
	c->Addr.sin_port         = CAPI_htons((unsigned short)tcpPort);
	c->Addr.sin_addr.s_addr  = CAPI_htonl(IPaddress);
		
	/* create a mutex */
	CREATE_LOCK(c->lock,"connect socket");
	REQUEST_LOCK(c->lock);
	c->state = COMAPI_STATE_CONNECTION_PENDING;
	RELEASE_LOCK(c->lock);
		
	/* Create thread which attempt to make connection */
	c->ThreadHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)RequestConnection,(LPVOID)c,0,&ConnectThread);

	if (c->ThreadHandle == NULL)
	{
//		DWORD error = GetLastError();	   
		ComTCPFreeData(c);
		return NULL;
	}
	
	return (ComAPIHandle)c;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ComAPIHandle ComTCPOpenAccept(unsigned long IPaddress, int tcpPort,int timeoutsecs )
{
	ComTCP *c, *listener;
	CAPIList *listitem;
	WSADATA wsaData;
	
	/* InitWS2 checks that WSASstartup is done only once and increments reference count*/
	if (InitWS2(&wsaData) == 0)
	{
		return NULL;
	}
	
	if (IPaddress == 0)
	{
		return NULL;
	}
	enter_cs();
	if(!(listitem = CAPIListFindTCPListenPort(GlobalListHead,(unsigned short)tcpPort)))
	{
	    leave_cs();
		return NULL;
	}
	
	listener = (ComTCP *)listitem->com;
	
	/* Is this handle already in our list */
	listitem = CAPIListFindTCPIPaddress(GlobalListHead,CAPI_htonl(IPaddress),CAPI_htons((unsigned short)tcpPort));
	if(listitem)
	{ 
	    leave_cs();
		
		return listitem->com;
	}
	
	/* add connection to our local list */
	GlobalListHead = CAPIListAppend(GlobalListHead);
	if(!GlobalListHead)
	{
	    leave_cs();
		return NULL;
	}
	
	/* allocate a new ComHandle struct */
	GlobalListHead->com = (ComAPIHandle)malloc(sizeof(ComTCP));
	c = (ComTCP*)(GlobalListHead->com);
	memset(c,0,sizeof(ComTCP));
	/* copy the Listen socket's ComHandle data into the Accepted socket's ComHandle*/
	memcpy(c,listener,sizeof(ComTCP));
	c->recv_sock = c->send_sock = 0;
	c->state = COMAPI_STATE_CONNECTION_PENDING;
	((ComAPIHandle)c)->protocol = CAPI_TCP_PROTOCOL;
	c->lock = 0;
	
	/* Create the target address... */
	memset ((char*)&c->Addr, 0, sizeof(c->Addr));
	c->Addr.sin_family       = AF_INET;
	c->Addr.sin_port         = CAPI_htons((unsigned short)tcpPort);
	c->Addr.sin_addr.s_addr  = CAPI_htonl(IPaddress);
	
	c->send_buffer.buf = (char *)malloc(c->buffer_size);
	c->recv_buffer.buf = (char *)malloc( c->buffer_size);
	c->recv_buffer_start = c->recv_buffer.buf;
	c->ThreadActive = THREAD_STOP;  
	c->handletype = CONNECTION;
	
	c->bytes_needed_for_message = (long)clock();  /* use this variable to hold start time */
	c->timeoutsecs = (short)timeoutsecs;
	
	    leave_cs();
	
	return (ComAPIHandle)c;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void AcceptConnection(LPVOID cvoid)
{
	ComTCP *ctcpListen = (ComTCP *)cvoid;   /* This is the handle to the listening socket */
	int err=0;
	struct sockaddr_in comCliAddr;
	int CliAddrLen;
	unsigned long trueValue = 1;
	SOCKET connecting_sock;
	ComTCP *ctcpAccept;
	
	AcceptConnectioncount++;
	
	CliAddrLen = sizeof(comCliAddr);
	
	/* A call to ComTCPClose will change ->ThreadActive so we can gracefully exit this thread */
	while (TRUE)
	{
		REQUEST_LOCK(ctcpListen->lock);
		if(ctcpListen->ThreadActive != THREAD_ACTIVE)
		{
			RELEASE_LOCK(ctcpListen->lock);
			break;
		}

		RELEASE_LOCK(ctcpListen->lock);
		
		/*	   Sleep(SLEEP_IN_ACCEPT);    */
		err = 0;
		
		/* wait for a connection */
		CliAddrLen = sizeof(comCliAddr);
		connecting_sock =  CAPI_accept (ctcpListen->recv_sock, (struct sockaddr*)&comCliAddr,&CliAddrLen);
		
		AcceptCount++;
		REQUEST_LOCK(ctcpListen->lock);
		
		/* if reasonable ERROR loop back and wait for another connection */
		if(connecting_sock == INVALID_SOCKET)
		{
			err = CAPI_WSAGetLastError(); 
			if(err == WSAEWOULDBLOCK)
			{
				CAPIList *listitem;

				enter_cs();

				listitem = CAPIListFindTCPAcceptPendingExpired(GlobalListHead);  

				leave_cs();

				if( listitem)
				{
					listitem->com->close_func(listitem->com);
					((ComTCP *)(listitem->com))->accept_callback_func(listitem->com,COMAPI_CONNECTION_TIMEOUT);
				}
				
				RELEASE_LOCK(ctcpListen->lock);
				
				continue;
			}
			else if(ctcpListen->ThreadActive != THREAD_ACTIVE && err == WSAENOTSOCK)
			{
				break;
			}
			else
			{
				RELEASE_LOCK(ctcpListen->lock);
				break;
			}
		}
		else
		{
			CAPIList *listitem;
			char *save_recv_buffer;
			char *save_send_buffer;
			
			save_recv_buffer  =  NULL;
			save_send_buffer  =  NULL;
			
			enter_cs();
			listitem = CAPIListFindTCPIPaddress(GlobalListHead,comCliAddr.sin_addr.s_addr,ctcpListen->Addr.sin_port);
			
			if (listitem)
			{
				ctcpAccept = (ComTCP *)listitem->com;
				save_recv_buffer =  ctcpAccept->recv_buffer.buf;
				save_send_buffer  = ctcpAccept->send_buffer.buf;
			}
			else
			{
				/*  Create a new entry in list of connections */
				
				GlobalListHead = CAPIListAppend(GlobalListHead);

				if(!GlobalListHead)
				{
				    leave_cs();
					return;
				}
				
				/* allocate a new ComHandle struct */
				GlobalListHead->com = (ComAPIHandle)malloc(sizeof(ComTCP));
				ctcpAccept = (ComTCP *)(GlobalListHead->com);
				memset(ctcpAccept,0,sizeof(ComTCP));
			}

			leave_cs();
			
			/* copy the Listen socket's ComHandle data into the Accepted socket's ComHandle*/
			memcpy(ctcpAccept,ctcpListen,sizeof(ComTCP));
			
			/* BUT  no thread on accepted sockets  */
			ctcpAccept->ThreadActive = THREAD_STOP;  
			ctcpAccept->handletype = CONNECTION;
			
			/* copy  who we are connected to */
			ctcpAccept->Addr.sin_addr=comCliAddr.sin_addr;
			
			/* set the socket numbers */
			ctcpAccept->recv_sock = ctcpAccept->send_sock = connecting_sock;
			
			/* allocate send and receive data buffers */
			if( save_send_buffer  ==  NULL)
			{
				ctcpAccept->send_buffer.buf = (char *)malloc(ctcpListen->buffer_size);
			}
			else 
			{
				ctcpAccept->send_buffer.buf = save_send_buffer;
			}

			if (save_recv_buffer  ==  NULL)
			{
				ctcpAccept->recv_buffer.buf = (char *)malloc( ctcpListen->buffer_size);
			}
			else
			{
				ctcpAccept->recv_buffer.buf = save_recv_buffer;
			}

			ctcpAccept->recv_buffer_start = ctcpAccept->recv_buffer.buf;
			ctcpAccept->referencecount = 1;
			
			/* Create a mutex for this connection */
			CREATE_LOCK(ctcpAccept->lock,"accept socket");
			
			/* Now set the new socket for Non-Blocking IO */
			if(err=CAPI_ioctlsocket(ctcpAccept->recv_sock, FIONBIO, &trueValue))
			{
				err = CAPI_WSAGetLastError();
			}
			
			/* turn off the Nagle alog. which buffers small messages */
			err = CAPI_setsockopt (ctcpAccept->recv_sock, IPPROTO_TCP,TCP_NODELAY, (char *)&trueValue, sizeof (trueValue));
			
			if (err)
			{
				err = CAPI_WSAGetLastError();
			}
			
			/* initialize header data values */
			initComTCP(ctcpAccept);
			
			/* Increment reference count of openconnections */
			/* we have to do this here because no explicit WS2Init() was called */
			WS2Connections++;
			
			/* Notify User thru his callback that we have connected on a socket */
			ctcpAccept->state = COMAPI_STATE_ACCEPTED;
			
			ctcpAccept->accept_callback_func((ComAPIHandle)ctcpAccept,0);
			
			/* Any errors ? */
			if ( err )
			{
				RELEASE_LOCK(ctcpListen->lock);
				break;			  
			}
			
			RELEASE_LOCK(ctcpListen->lock);
		}
	}

	/* If we get here we need to exit to terminate the thread */

	REQUEST_LOCK(ctcpListen->lock); 

	if(ctcpListen->ThreadActive == THREAD_ACTIVE)  /* we got here due to a break */
	{
		/* indicate that thread is inactive, about to exit */
		ctcpListen->ThreadActive = THREAD_STOP;
		RELEASE_LOCK(ctcpListen->lock);
		
	}
	else
	{
		/* indicate that we are exiting to ComTCPClose which is waiting for exit of thread*/
		ctcpListen->ThreadActive = THREAD_TERMINATED;
		RELEASE_LOCK(ctcpListen->lock);
	}

	return;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void RequestConnection(LPVOID cvoid)
{
	ComTCP *c = (ComTCP *)cvoid;
	int err;
	int CliAddrLen;
	unsigned long trueValue = 1;
	int FirstWouldblock;
	int error;
	clock_t starttime,endtime;
	
	CliAddrLen = sizeof(c->Addr);
	
	/* set the new socket for Non-Blocking IO */
	if(err=CAPI_ioctlsocket(c->recv_sock, FIONBIO, &trueValue))
	{
		err = CAPI_WSAGetLastError();
	}
	
	FirstWouldblock=1;
	starttime = clock();
	c->state = COMAPI_STATE_CONNECTION_PENDING;
	
	while(TRUE)
	{
		Sleep(0);
		
		/* Look for a connection */
		if(CAPI_connect(c->recv_sock, (struct sockaddr*)&c->Addr, CliAddrLen))
		{
			error = CAPI_WSAGetLastError();

			if(error == WSAEWOULDBLOCK)
			{
				FirstWouldblock = 0;
			}
			
			/* after the first WSAEWOULDBLOCK, WASEISCONN means successfull connection */
			if(error == WSAEISCONN && !FirstWouldblock)  /* got a good connection */
			{
				break;
			}

			/* check for timeout */
			endtime = (clock() - starttime)/1000;
			
			/* keep waiting */
			if(error == WSAEWOULDBLOCK || error == WSAEALREADY)
			{
				if(endtime  < c->timeoutsecs)
				{
					continue;
				}
				else
				{
					/* error or time out here , so inform user and then exit*/
					c->state = COMAPI_STATE_CONNECTED;
					c->connect_callback_func((ComAPIHandle)c, -1 * error);

					return;
				}
			}
			else
			{
				if ((error == WSAEINVAL) && (endtime  < c->timeoutsecs))
				{
					continue;
				}

				/* error or time out here , so inform user and then exit*/
				c->state = COMAPI_STATE_CONNECTED;
				c->connect_callback_func((ComAPIHandle)c,-1 * error);

				return;
			}
		}
		else  /* 0 means successful connection */
		{
			break;
		}
	}
	
	/* If we get here we have a good connection */
	/* Set send socket to the same */
	c->send_sock = c->recv_sock;
		
	err = CAPI_setsockopt (c->recv_sock, IPPROTO_TCP,TCP_NODELAY,(char *)&trueValue,sizeof (trueValue));
	
	if (err)
	{
		err = CAPI_WSAGetLastError();
	}
	
	/* initialize some data */
	initComTCP(c);
	
	REQUEST_LOCK(c->lock);

	c->state = COMAPI_STATE_CONNECTED;

	RELEASE_LOCK(c->lock);
	
	/* Notify User that we have connected on a socket */
	c->connect_callback_func((ComAPIHandle)c,0);
	
	return;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ComTCPClose(ComAPIHandle c)
{
	int sockerror;
	ComTCP *ctcp = (ComTCP *)c;
	
	if(!c)
	{
		return;
	}
	
	enter_cs();
	
	if(!CAPIListFindHandle(GlobalListHead,c))
	{
		leave_cs();

		return ; /* is it in  our list ? */
	}

	leave_cs();
	
	ctcp->referencecount --;

	if (ctcp->referencecount)
	{
		return;
	}
	
	if(ctcp->lock)
	{
		REQUEST_LOCK(ctcp->lock);
	}
	
	if (ctcp->recv_sock > 0)
	{
		/* if this is a listening socket, we have an active listening thread*/
		if(ctcp->ThreadActive == THREAD_ACTIVE)
		{
			ctcp->ThreadActive = THREAD_STOP;   /* tell thread to stop */

			RELEASE_LOCK(ctcp->lock);
			
			/* close the socket */
			if(sockerror = CAPI_closesocket(ctcp->recv_sock))
			{
				sockerror = CAPI_WSAGetLastError();
			}

			ctcp->recv_sock =0;  /* clear it */
			
			while(TRUE)
			{
				REQUEST_LOCK(ctcp->lock);

				if (ctcp->ThreadActive == THREAD_TERMINATED)
				{
					break;
				}

				RELEASE_LOCK(ctcp->lock);
			}
		}
		else  /* not a listening socket , a regular socket */
		{
			/* close the socket */
			if(sockerror = CAPI_closesocket(ctcp->recv_sock))
			{
				sockerror = CAPI_WSAGetLastError();
			}

			ctcp->recv_sock =0;  /* clear it */
		}
	}	
		
	/* reset this to start of receive buffer so pointer to free() is correct */
	ctcp->recv_buffer.buf = ctcp->recv_buffer_start;  
	
	/* remove this ComHandle from the current list */
	enter_cs();

	GlobalListHead = CAPIListRemove( GlobalListHead, c );

	leave_cs();
	
	if(ctcp->lock)
	{
		RELEASE_LOCK(ctcp->lock);

		DESTROY_LOCK(ctcp->lock);
	}

	ctcp->lock = 0;  /* clear it */
	
	if ((ctcp->handletype != GROUP) && (ctcp->state != COMAPI_STATE_CONNECTION_PENDING))
	{
		WS2Connections--;   /* decrement the INIT reference count */
	}
		
	/* free the data structs */
	if (ctcp->recv_buffer.buf)
	{
		free(ctcp->recv_buffer.buf);
	}

	if (ctcp->send_buffer.buf)
	{
		free(ctcp->send_buffer.buf);
	}

	free(c);
	
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
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ComTCPSendX(ComAPIHandle c, int msgsize, int oob, int type, ComAPIHandle Xcom)
{
   if (c == Xcom){
	   return 0;
   }
   else {
	   return ComTCPSend(c, msgsize, oob, type);
   }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ComTCPSend(ComAPIHandle c, int msgsize, int oob, int type)
{
	if(c)
	{
		ComTCP *ctcp = (ComTCP *)c;
		int senderror,err;
		int bytesSent;
		
		enter_cs();

		if(!CAPIListFindHandle(GlobalListHead,c))
		{
			leave_cs();

			return  -1 * WSAENOTSOCK; /* is it in  our list ? */
		}

		leave_cs();

		if(ctcp->handletype == LISTENER)
		{
			return -1 * WSAENOTSOCK;
		}
		
		if(ctcp->state == COMAPI_STATE_CONNECTION_PENDING)
		{
			return COMAPI_CONNECTION_PENDING;
		}
		
		if(msgsize > ctcp->buffer_size - (int)sizeof(tcpHeader))
		{
			return COMAPI_MESSAGE_TOO_BIG;
		}
		
		err = 0;
		/* add the header size to the requested messagesize */
		ctcp->send_buffer.len = msgsize + ctcp->headersize;
		/* create and store header in buffer */
		setHeader(ctcp->send_buffer.buf,msgsize);

		if (check_bandwidth(ctcp->send_buffer.len, 1, type))
		{
			bytesSent = senderror =  CAPI_send
			(
				ctcp->send_sock,
				ctcp->send_buffer.buf,
				ctcp->send_buffer.len,
				0
			);
		}
		else
		{
			return COMAPI_WOULDBLOCK;
		}
		
		if (senderror == SOCKET_ERROR)
		{
			senderror = CAPI_WSAGetLastError();
			
			if(senderror == WSAEWOULDBLOCK) 
			{
				/* let's keep a count of wouldblocks - might be interesting later */
				
				ctcp->sendwouldblockcount++;
				return COMAPI_WOULDBLOCK;   
			}
			
			return -1 * senderror;
		}
		
		use_bandwidth (bytesSent, 1, type);
#ifdef checkbandwidth

totalbwused += bytesSent;
if (oob) oobbwused +=bytesSent;
if (now > laststatcound + 1000)
{
	MonoPrint("TCP Bytes pr sec %d, OOB %d", 
		(int)(totalbwused*1000/(now-laststatcound)),
		(int)(oobbwused *1000/(now-laststatcound)));
	laststatcound = now;
	totalbwused = 0;
	oobbwused = 0;
	Posupdbwused = 0;
}
#endif		
		if(bytesSent != (int)ctcp->send_buffer.len )  /* Incomplete message was sent?? */
		{
			ctcp->sendwouldblockcount++;
			return COMAPI_WOULDBLOCK;
		}
		else
		{
			/* keep track of successful messages sent/received */
			ctcp->sendmessagecount++;
			return bytesSent - ctcp->headersize;
		}
	}
	else
	{
		return -1 * WSAENOTSOCK;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char *ComTCPSendBufferGet(ComAPIHandle c)
{
	if(c)
	{
		enter_cs();

		if(!CAPIListFindHandle(GlobalListHead,c))
		{
			leave_cs();

			return  NULL; /* is it in  our list ? */
		}

		leave_cs();
		
		return ((ComTCP *)c)->send_buffer.buf + sizeof(tcpHeader);
	}
	else
	{
		return NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char *ComTCPRecvBufferGet(ComAPIHandle c)
{
	if(c)
	{
		enter_cs();

		if (!CAPIListFindHandle(GlobalListHead,c))
		{
			leave_cs();

			return NULL; /* is it in  our list ? */
		}

		leave_cs();
		
		if (((ComTCP *)c)->handletype == GROUP)
		{
			return NULL;
		}
		
		return ((ComTCP *)c)->recv_buffer_start +sizeof(tcpHeader) ;
	}
	else
	{
		return NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

unsigned long ComTCPQuery(ComAPIHandle c, int querytype)
{
	if(c)
	{
		if (((ComTCP *)c)->handletype == GROUP)
		{
			return 0;
		}
		
		switch(querytype)
		{
			case COMAPI_MESSAGECOUNT:
				return ((ComTCP *)c)->sendmessagecount + ((ComTCP *)c)->recvmessagecount;
				break;

			case COMAPI_SEND_MESSAGECOUNT:
				return ((ComTCP *)c)->sendmessagecount;
				break;

			case COMAPI_RECV_MESSAGECOUNT:
				return ((ComTCP *)c)->recvmessagecount;
				break;

			case COMAPI_RECV_WOULDBLOCKCOUNT:
				return ((ComTCP *)c)->recvwouldblockcount;
				break;

			case COMAPI_SEND_WOULDBLOCKCOUNT:
				return ((ComTCP *)c)->sendwouldblockcount;
				break;

			case COMAPI_CONNECTION_ADDRESS:
			case COMAPI_SENDER:
				return CAPI_ntohl(((ComTCP *)c)->Addr.sin_addr.s_addr);
				break;

			case COMAPI_RECEIVE_SOCKET:
				return ((ComTCP *)c)->recv_sock;
				break;

			case COMAPI_SEND_SOCKET:
				return ((ComTCP *)c)->send_sock;
				break;

			case COMAPI_RELIABLE:
				return 1;
				break;

			case COMAPI_MAX_BUFFER_SIZE:
				return  0;
				break;

			case COMAPI_ACTUAL_BUFFER_SIZE:
				return ((ComIP *)c)->buffer_size - sizeof(tcpHeader);
				break;

			case COMAPI_PROTOCOL:
				return  c->protocol;
				break;

			case COMAPI_STATE:
				return  ((ComTCP *)c)->state;
				break;

			case COMAPI_TCP_HEADER_OVERHEAD:
				return sizeof(tcpHeader) + 40;	// Size of underlying header.
				break;
				
			default:
				return 0;
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ComTCPRecv(ComAPIHandle c, int BytesToRecv)
{
	if(c)
	{
		ComTCP *ctcp = (ComTCP *)c;
		int recverror;
		int bytesRecvd;
//		int flags = 0;
		
		ctcp->recv_buffer.len = BytesToRecv;
		
		/* precautionary test to protect buffers, since recv_buffer.buf is updated in ComTCPGetNBytes() */
		if (ctcp->recv_buffer.buf + ctcp->recv_buffer.len > ctcp->recv_buffer_start + ctcp->buffer_size)
		{
			return COMAPI_OVERRUN_ERROR;
		}
		
		/* now perform the the receive */
		bytesRecvd = CAPI_recv(ctcp->recv_sock,ctcp->recv_buffer.buf,ctcp->recv_buffer.len,0);
		
		recverror = 0;

		if(bytesRecvd == 0 )
		{
			recverror = COMAPI_CONNECTION_CLOSED;      /* graceful close is indicated */
		}
		else if(bytesRecvd == SOCKET_ERROR)
		{
			recverror = CAPI_WSAGetLastError();      /* error condition */
		}
		
		if(recverror)
		{
			if(recverror == WSAEWOULDBLOCK )     /* nothing ready */
			{
				recverror = 0;
				ctcp->recvwouldblockcount++;
			}
			else if (recverror != COMAPI_CONNECTION_CLOSED)
			{
				recverror *= -1;                 /* Negate Winsock error code */
			}

			return recverror;
		}
		else
		{
			return bytesRecvd;
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

int ComTCPGetMessage(ComAPIHandle c)
{
//	int retval = 0;
	
	if(c)
	{
		ComTCP *ctcp = (ComTCP *)c;
		int bytesRecvd;
		
		if(ctcp->handletype == GROUP)
		{
			return -1 * WSAENOTSOCK;
		}

		if(ctcp->state == COMAPI_STATE_CONNECTION_PENDING)
		{
			return COMAPI_CONNECTION_PENDING;
		}

		enter_cs();

		if(!CAPIListFindHandle(GlobalListHead,c))
		{
			leave_cs();
			return  -1 * WSAENOTSOCK; /* is it in  our list ? */
		}

		leave_cs();
		
		if(ctcp->handletype == LISTENER)
		{
			return -1 * WSAENOTSOCK;
		}
		
		/* Make this thread safe */
		REQUEST_LOCK(ctcp->lock); 
		
		/* need to read a header for message size */
		if (ctcp->bytes_needed_for_header)  
		{
			bytesRecvd = ComTCPGetNbytes(c,ctcp->bytes_needed_for_header);

			if (bytesRecvd <= 0 )    /* either no data (= 0) available or an error */
			{
				RELEASE_LOCK(ctcp->lock);
				return bytesRecvd;
			}

			ctcp->bytes_needed_for_header -= bytesRecvd;

			if (ctcp->bytes_needed_for_header == 0 )  /* we now have a complete header */
			{
				/*should have a header here for new message .. so extract size of message*/
				ctcp->messagesize = isHeader(ctcp->recv_buffer_start);   

				if (ctcp->messagesize == 0 )
				{
					RELEASE_LOCK(ctcp->lock);
					return COMAPI_BAD_HEADER;
				}

				/* now set these values for receiving the message data */
				ctcp->bytes_needed_for_message = ctcp->messagesize; 
				ctcp->bytes_recvd_for_message = 0; 
			}
			else  /* incomplete header .. try again later */
			{
				RELEASE_LOCK(ctcp->lock);
				return 0;  
			}
		}
		
		/* Do we need to receive  message data ? */
		if (ctcp->bytes_needed_for_message)
		{
			if(ctcp->bytes_needed_for_message > ctcp->buffer_size - (int)sizeof(tcpHeader))
			{ 
				RELEASE_LOCK(ctcp->lock);
				return COMAPI_MESSAGE_TOO_BIG;
			}

			bytesRecvd = ComTCPGetNbytes(c,ctcp->bytes_needed_for_message);

			if (bytesRecvd <= 0 )    /* either no data (= 0) available or an error */
			{
				RELEASE_LOCK(ctcp->lock);
				return bytesRecvd;
			}

			ctcp->bytes_needed_for_message -= bytesRecvd;
			ctcp->bytes_recvd_for_message  += bytesRecvd;

			if (ctcp->bytes_needed_for_message == 0 )  /* we now have a complete message */
			{
				/* a PANIC Check */
				if (ctcp->messagesize != ctcp->bytes_recvd_for_message)
				{
					RELEASE_LOCK(ctcp->lock);
					return COMAPI_BAD_MESSAGE_SIZE;
				}
				
				/* Reset for next message */
				ctcp->bytes_needed_for_header = ctcp->headersize;
				ctcp->bytes_needed_for_message = 0;
				ctcp->bytes_recvd_for_message = 0;
				ctcp->recv_buffer.buf = ctcp->recv_buffer_start; 
				ctcp->recvmessagecount++;    /* global message counter */
				RELEASE_LOCK(ctcp->lock);

				if(CAPI_TimeStamp)
				{
					ctcp->timestamp = CAPI_TimeStamp();
				}

				return ctcp->messagesize;
			}    
			else     /* incomplete message */
			{
				RELEASE_LOCK(ctcp->lock);
				return 0;   // complete message not avaliable yet
			}
		}
		
		RELEASE_LOCK(ctcp->lock);

		return 0;   /* message not avaliable yet */
	}
	
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void initComTCP(ComTCP *c)
{
	c->headersize = sizeof(tcpHeader);
	c->bytes_needed_for_header = c->headersize;
	c->bytes_needed_for_message = 0;
	
	return;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int isHeader(char *data) 
{
	unsigned int ret=0;
	
	if(data)
	{
		tcpHeader *header;
		unsigned long *lptr;
		
		lptr = (unsigned long *)data;
		header = (tcpHeader *)data;
		ret = 1;
		
		if (header->header_base != HEADER_BASE)
		{
			ret = 0;
		}
		else
		{
			ret =  ((header->inv_size ^ header->size) == 0xFFFF) ? header->size : 0;
		}
	}
	
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void setHeader(char *data, int size)
{
	tcpHeader *header;
	
	if (data)
	{
		header = (tcpHeader *)data;
		header->header_base = HEADER_BASE;
		header->size = (unsigned short)size;
		header->inv_size = (unsigned short)(~size);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ComTCPGetNbytes(ComAPIHandle c, int BytesToGet)
//static int ComTCPGetNbytes(ComAPIHandle c, int BytesToGet)
{
	ComTCP *ctcp = (ComTCP *)c;
	int bytesRecvd=1;
	int BytesGotten=0;
	
	if(c)
	{
		//	while(BytesGotten < BytesToGet && bytesRecvd > 0)
		while(BytesToGet && bytesRecvd > 0)
		{
			bytesRecvd = ComTCPRecv(c,BytesToGet);
			
			/* If bytesRecvd == 0 means nothing to get .. so quit */
			if (bytesRecvd > 0)
			{
				BytesGotten += bytesRecvd;
				BytesToGet -= bytesRecvd;
				ctcp->recv_buffer.buf += bytesRecvd;  /* move the receive buffer start point along */
			}
			else
			{
				if (bytesRecvd < 0)
				{
					BytesGotten = bytesRecvd; /*negative error back  thru BytesGotten */
				}
			}
		}
	}

	return BytesGotten;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void ComTCPFreeData(ComTCP *c)
{
	if(c)
	{
		leave_cs();

		GlobalListHead = CAPIListRemove(GlobalListHead,(ComAPIHandle)c);
		
		leave_cs();

		/* free the data structs */
		if(c->recv_buffer.buf)
		{
			free(c->recv_buffer.buf);
		}

		if(c->send_buffer.buf)
		{
			free(c->send_buffer.buf);
		}

		free(c);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int CAPIListCount (CAPIList *list)
{
	CAPIList *curr;
	int i;
	
	if(!list)
	{
		return 0;
	}
	
	i = 0;
	curr = list; 
	while (curr)
	{
		i++;
		curr = curr->next;
	}
	
	return i;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ComAPICloseOpenHandles (void)
{
	CAPIList * curr, *list;
	int i;
	
	list = GlobalGroupListHead;
	
	if(list) 
	{
		for( i = 0, curr = list; curr; i++, curr = curr  ->  next ) 
		{
			curr->com->close_func(curr->com); 
		}
	}  
	
	list = GlobalListHead;

	if(list) 
	{
		for( i = 0, curr = list; curr; i++, curr = curr  ->  next ) 
		{
			curr->com->close_func(curr->com); 
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

CAPIList *CAPIListAppend( CAPIList * list )
{
	CAPIList * newnode;
	
	newnode = (CAPIList *)malloc(sizeof(CAPIList));
	memset(newnode,0,sizeof(CAPIList));
	newnode -> com = NULL;
	newnode -> next = list;
	
	MonoPrint ("%08x = CAPI List Append %08x\n", newnode, list);
	return newnode;
}                         

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

CAPIList *CAPIListRemove( CAPIList * list, ComAPIHandle com )
{
	CAPIList
		*start,
		*prev,
		*curr;

	start = list;

	if (!list)
	{
		return NULL;
	}

	while (1)
	{
		prev = NULL;
		curr = list;
		
		while ((curr) && (curr -> com != com))
		{
			prev = curr;
			curr = curr -> next;
		}
		
		/* not found, return list unmodified */
		if (!curr)
		{
#ifdef _DEBUG
//			if (list)
//				MonoPrint ("%08x = CAPI List Remove %08x \"%s\" %08x\n", list, start, com->name, ((ComIP*)com)->address.sin_addr.s_addr);
#endif

			return list;
		}

		/* found at head */
		if (!prev)
		{
			list = list -> next;
		}
		else
		{
			prev -> next = curr -> next;
		}
		
		curr -> next = NULL;
		
		free (curr);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

CAPIList *CAPIListFindHandle( CAPIList * list, ComAPIHandle com )
{
	CAPIList * curr;
	
	for( curr = list; curr; curr = curr -> next ) 
	{
		if ( curr -> com == com )
		{
			return curr;
		}
	}
	
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

CAPIList *CAPIListFindTCPListenPort( CAPIList * list, short port )
{
	CAPIList * curr;
	
	if(port == 0)
	{
		return NULL;
	}
	
	for (curr = list; curr; curr = curr -> next) 
	{
		if (curr->com->protocol == CAPI_TCP_PROTOCOL)
		{
			if ((((ComTCP *)(curr -> com ))->ListenPort == port) && (((ComTCP *)(curr -> com ))->handletype == LISTENER))
			{
				return curr;
			}
		}
	}
	
	return NULL ;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

CAPIList *CAPIListFindTCPIPaddress(CAPIList * list, unsigned long IPaddress, unsigned short tcpPort)
{
	CAPIList * curr;
	ComTCP *c;
	
	if (IPaddress == 0)
	{
		return NULL;
	}
	
	for (curr = list; curr; curr = curr -> next)
	{
		if (curr->com->protocol == CAPI_TCP_PROTOCOL)
		{
			unsigned long cip;

			c = (ComTCP *)curr->com;
			cip = c->Addr.sin_addr.s_addr;

			if ((cip == IPaddress) && (c->Addr.sin_port == tcpPort))
			{
				return curr;
			}
		}
	}
	
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static CAPIList *CAPIListFindTCPAcceptPendingExpired (CAPIList * list)
{
	CAPIList * curr;
	ComTCP *c;
	
	for (curr = list; curr; curr = curr -> next)
	{
		if (curr->com->protocol == CAPI_TCP_PROTOCOL)
		{
			c = (ComTCP *)curr->com;

			if ((c->state == COMAPI_STATE_CONNECTION_PENDING) && (c->timeoutsecs))
			{
				if (c->timeoutsecs <= ((clock() - c->bytes_needed_for_message)/1000))
				{
					c->timeoutsecs = 0;

					return curr;
				}
			}
		}
	}
	
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CAPIListDestroy( CAPIList * list, void (* destructor)() )
{
	CAPIList 
		*prev,
		*curr;
	
	if (!list)
	{
		return;
	}
	
	prev = list;
	curr = list -> next;
	
	while (curr)
	{
		if (destructor)
		{
			(*destructor)(prev -> com);
		}

		prev -> next = NULL;
		
		free (prev);
		
		prev = curr;
		curr = curr -> next;
	}
	
	if (destructor)
	{
		(*destructor)(prev -> com);
	}
	
	prev -> next = NULL;
	
	free( prev );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

CAPIList *CAPIListAppendTail( CAPIList * list )
{
	CAPIList * newnode;
	
	newnode = (CAPIList *)malloc(sizeof(CAPIList));
	memset(newnode,0,sizeof(CAPIList));
	
	/* list was null */
	if ( !list ) 
	{
		list = newnode;
	}
	else 
	{
		list -> next = newnode;
	}

	MonoPrint ("%08x = CAPI List Append Tail %08x\n", newnode, list);
	return newnode;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char *ComGROUPRecvBufferGet(ComAPIHandle c)
{
	c;

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


int ComGROUPGet(ComAPIHandle c)
{
	c;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// JPO -always called from within the CS
void ComGROUPClose(ComAPIHandle c)
{
	CAPIList *curr;
	
	if(!c)
	{
		return;
	}
	
	if(!CAPIListFindHandle(GlobalGroupListHead,c))           /* in our list of groups ?*/
	{
		return;
	}
	
	CAPIListDestroy(((ComGROUP *)c)->GroupHead,NULL);                /* destroy this group list */
	
#ifdef _DEBUG
	MonoPrint ("Group Close CH:\"%s\"\n", c->name);
#endif
	GlobalGroupListHead = CAPIListRemove(GlobalGroupListHead,c);     /* remove group from list of groups */
	
#ifdef _DEBUG
	MonoPrint ("================================\n");
	MonoPrint ("GlobalGroupListHead\n");
	
	curr = GlobalGroupListHead;
	
	while (curr){
		if (!F4IsBadReadPtrC(curr->com->name, 1)) // JB 010724 CTD
			MonoPrint ("  \"%s\"\n", curr->com->name);
		curr = curr->next;
	}
	
	MonoPrint ("================================\n");
#endif
	
	for( curr = GlobalGroupListHead ; curr  != NULL ; curr = curr -> next )
	{
		ComAPIDeleteFromGroup(curr->com,c);                              /* remove this group from others */
	}
	
	if(((ComGROUP *)c)->send_buffer)
	{
		free(((ComGROUP *)c)->send_buffer);
	}
	
	free(c);
	
	return;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

unsigned long  ComGROUPQuery(ComAPIHandle c, int querytype)
{
	if(c)
	{
		switch(querytype)
		{
			case COMAPI_PROTOCOL:
				return  c->protocol;
			
			case COMAPI_ACTUAL_BUFFER_SIZE:
				return ((ComGROUP *)c)->buffer_size -  ((ComGROUP *)c)->max_header;
				
			default:
				return 0;
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int  ComGROUPAddress(ComAPIHandle c, char *buf, int reset)
{
	unsigned long ipaddr;

	if(c)
	{
		ipaddr = ((ComGROUP *)c)->HostID;

		if (ipaddr)
		{
			*((int*)buf) = *((int*)(&ipaddr));
			return 0;
		}
	}

	return COMAPI_HOSTID_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ComAPIHandle CAPIIsInGroup (ComAPIHandle grouphandle, unsigned long ipAddress)
{
	ComGROUP *group = (ComGROUP *)grouphandle;
	CAPIList * curr;
	
	if(!grouphandle)
	{
		return NULL;
	}

	if(!CAPIListFindHandle(GlobalGroupListHead,grouphandle))
	{
		return NULL; /* is it in  our list ? */
	}

	if (group->GroupHead == NULL)
	{
		return NULL;
	}
	
	/* proceed thru list and call send_function() for each connection */      
	for( curr = group->GroupHead; curr; curr = curr -> next ) 
	{
		if(curr->com->protocol == CAPI_TCP_PROTOCOL)
		{
			ComTCP *c;
			unsigned long cip;
			
			c = (ComTCP *)curr->com;
			cip = CAPI_ntohl(c->Addr.sin_addr.s_addr);

			if ( cip == ipAddress)
			{
				return curr->com;
			}
		}
		else if(curr->com->protocol == CAPI_UDP_PROTOCOL)
		{
			ComIP *c;
			unsigned long cip;
			
			c = (ComIP *)curr->com;
			cip = CAPI_ntohl(c->sendAddress.sin_addr.s_addr);

			if ( cip == ipAddress)
			{
				return curr->com;
			}
		}
		else if
		(
			(curr->com->protocol == CAPI_DPLAY_TCP_PROTOCOL)   || 
			(curr->com->protocol == CAPI_DPLAY_MODEM_PROTOCOL) ||
			(curr->com->protocol == CAPI_DPLAY_SERIAL_PROTOCOL)
		)
		{
			unsigned long cip;
			cip = ComAPIQuery(curr->com,COMAPI_CONNECTION_ADDRESS);

			if ( cip == ipAddress)
			{
				return curr->com;
			}
		}
		else if (curr->com->protocol == CAPI_GROUP_PROTOCOL)  /* another group */
		{
			ComAPIHandle c;
			c = CAPIIsInGroup(curr->com,ipAddress);
			if (c)
			{
				return c;
			}
		}
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int  ComAPIAddToGroup(ComAPIHandle grouphandle, ComAPIHandle memberhandle )
{
	ComGROUP *group = (ComGROUP *)grouphandle;
	CAPIList *curr = 0;
	
	if(!grouphandle)
	{
		return COMAPI_EMPTYGROUP;
	}

	if(grouphandle->protocol != CAPI_GROUP_PROTOCOL)
	{
		return COMAPI_NOTAGROUP;
	}

	enter_cs ();
	
	group->GroupHead = CAPIListRemove(group->GroupHead,memberhandle);
	group->GroupHead = CAPIListAppend(group->GroupHead);

	if(! group->GroupHead)
	{
		leave_cs ();
		return COMAPI_EMPTYGROUP;
	}

	group->GroupHead->com = memberhandle;

	/* Reduce group send_buffer size if adding a member with smaller buffer */
	{	
		int bufSize = group->buffer_size;
		int qSize = (int)memberhandle->query_func(memberhandle,COMAPI_ACTUAL_BUFFER_SIZE)+group->max_header;
		group->buffer_size = min(bufSize, qSize);
	}
	
	/* If Group HostID is NULL, take HostID from the first added member*/
	if(group->HostID == 0){
		memberhandle->addr_func(memberhandle,(char *)&group->HostID, 0);
	}

	leave_cs ();

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ComAPIDeleteFromGroup(ComAPIHandle grouphandle, ComAPIHandle memberhandle )
{
	ComGROUP *group = (ComGROUP *)grouphandle;
	CAPIList
		*curr = 0;
	
	if(!grouphandle)
	{
		return 0;
	}

	if(!memberhandle)
	{
		return 0;
	}

	if(grouphandle->protocol != CAPI_GROUP_PROTOCOL)
	{
		return COMAPI_NOTAGROUP;
	}

	enter_cs ();
	
#ifdef _DEBUG
	MonoPrint ("ComAPIDeleteFromGroup GH:\"%s\" MH:\"%s\"\n", grouphandle->name, memberhandle->name);
#endif
	group->GroupHead = CAPIListRemove(group->GroupHead,memberhandle);

#ifdef _DEBUG
	MonoPrint ("================================\n");
	MonoPrint ("GROUP \"%s\"\n", grouphandle->name);
	curr = group->GroupHead;

	while (curr){
		if (!F4IsBadReadPtrC(curr->com->name, 1)) // JB 010724 CTD
			MonoPrint ("  \"%s\"\n", curr->com->name);
		curr = curr->next;
	}
	
	MonoPrint ("================================\n");
#endif

	leave_cs ();
	
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* get the associated write buffer */

char *ComGROUPSendBufferGet(ComAPIHandle c)
{
	if(c)
	{
		if(c->protocol != CAPI_GROUP_PROTOCOL)
		{
			return NULL;
		}

		return (((ComGROUP *)c)->send_buffer + ((ComGROUP *)c)->max_header);
	}
	else
	{
		return NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ComGROUPSend(ComAPIHandle c, int msgsize, int oob, int type)
{
	int senderror=0;
	int bytesSent=0;

	// JPO added a bunch of asserts after crashes occured in here.
	if(c){
		ComGROUP  *group = (ComGROUP *)c;
		CAPIList * curr;
		int ret = 0;
		int count = 0;
		char *save_send_buffer;
		enter_cs(); // JPO
		if(!CAPIListFindHandle(GlobalGroupListHead,c)){
		    leave_cs();
			return COMAPI_NOTAGROUP; /* is it in  our list ? */
		}
		
		if (group->GroupHead == NULL){
			senderror = COMAPI_EMPTYGROUP;
		}
		else {
			senderror = 0;
		}
	
		/* proceed thru list and call send_function() for each connection */      
		for (curr = group->GroupHead; curr; curr = curr->next){

			// @todo sfr: remove hack
			if (F4IsBadReadPtrC(curr->com, sizeof(ComAPIHandle))){
				// JB 010221 CTD
				continue; // JB 010221 CTD
			}

			if(curr->com->protocol == CAPI_TCP_PROTOCOL){
				ComTCP *this_ctcp;
				this_ctcp = (ComTCP *)curr->com;

				//if(this_ctcp)
				if (this_ctcp && !IsBadCodePtr((FARPROC) (*curr->com->send_func))) // JB 010401 CTD
				{
					save_send_buffer = this_ctcp->send_buffer.buf;
					this_ctcp->send_buffer.buf = group->send_buffer + group->TCP_buffer_shift;

					if (curr->com->send_func){
						ret = curr->com->send_func(curr->com, msgsize, oob, type);
					}

					this_ctcp->send_buffer.buf = save_send_buffer ;
				}
			}
			else if(curr->com->protocol == CAPI_UDP_PROTOCOL){
				ComIP *this_cudp;
				this_cudp = (ComIP*)curr->com;

				//if(this_cudp) // JB 010222 CTD
				if (
					this_cudp  && (this_cudp->send_buffer.buf) && 
					!F4IsBadReadPtrC(this_cudp->send_buffer.buf, sizeof(char)) && // JB 010222 CTD
					!F4IsBadCodePtrC((FARPROC)(*curr->com->send_func))
					 // JB 010401 CTD
				){
					save_send_buffer = this_cudp->send_buffer.buf;
					this_cudp->send_buffer.buf = group->send_buffer + group->UDP_buffer_shift;

					memcpy(this_cudp->send_buffer.buf,save_send_buffer,sizeof(ComAPIHeader));

					if (curr->com->send_func){
						ret = curr->com->send_func(curr->com, msgsize, oob, type);

						if (ret > 0){
//							oob = TRUE; // force other UDP packets to go OOB if the first one went
						}
					}

					this_cudp->send_buffer.buf = save_send_buffer ;
				}
			}
			else if (curr->com->protocol == CAPI_RUDP_PROTOCOL){
				ComIP *this_cudp;
								
				this_cudp = (ComIP *)curr->com;

				//if(this_cudp)
				if (this_cudp && !IsBadCodePtr((FARPROC) (*curr->com->send_func))) // JB 010401 CTD
				{
					save_send_buffer = this_cudp->send_buffer.buf;
					this_cudp->send_buffer.buf = group->send_buffer + group->RUDP_buffer_shift;
	
					if (curr->com->send_func)
					{
						ret = curr->com->send_func(curr->com, msgsize, oob, type);
					}

					this_cudp->send_buffer.buf = save_send_buffer ;
				}
			}
			/*else if
			(
				(curr->com->protocol == CAPI_DPLAY_TCP_PROTOCOL) || 
				(curr->com->protocol == CAPI_DPLAY_MODEM_PROTOCOL) ||
				(curr->com->protocol == CAPI_DPLAY_SERIAL_PROTOCOL)
			)
			{
				ret = ComDPLAYSendFromGroup(curr->com,msgsize,group->send_buffer +group->max_header);
			}*/
			else if(curr->com->protocol == CAPI_GROUP_PROTOCOL)  /* another group */
			{
				ComGROUP *this_group;
				this_group = (ComGROUP *) curr->com;

				//if(this_group)
				if(this_group && !F4IsBadCodePtrC((FARPROC) (*curr->com->send_func))) // JB 010401 CTD
				{
					save_send_buffer = this_group->send_buffer;
					this_group->send_buffer = group->send_buffer; 

					if (curr->com->send_func){
						ret = curr->com->send_func(curr->com, msgsize, FALSE, type);
					}

					this_group->send_buffer = save_send_buffer ;
				}
			}
			else {
				//me123 hack hack
				if (bytesSent == 0) ret = 1;
			}
			
			if( ret >= 0)
			{
				bytesSent += ret;
			}
			else
			{
				senderror = ret;
			}
		}
	}
	else {
		senderror = COMAPI_EMPTYGROUP;
	}
	leave_cs();

	if (senderror){
		return senderror;
	}
	else {
		return bytesSent;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ComGROUPSendX(ComAPIHandle c, int msgsize, int oob, int type, ComAPIHandle Xcom)
{
	int senderror=0;
	int bytesSent=0;
	
	if(c)
	{
		ComGROUP  *group = (ComGROUP *)c;
		CAPIList * curr;
		int ret = 0;
		char *save_send_buffer;
		enter_cs();
		if (!CAPIListFindHandle(GlobalGroupListHead,c))
		{
		    leave_cs();
			return COMAPI_NOTAGROUP; /* is it in  our list ? */
		}
		
		if (group->GroupHead == NULL)
		{
			senderror = COMAPI_EMPTYGROUP;
		}
		else
		{
			senderror = 0;
		}
		
		/* proceed thru list and call send_function() for each connection */      
		for (curr = group->GroupHead; curr; curr = curr -> next) 
		{
			if (curr->com == Xcom)
			{
				continue;
			}
			
			if(curr->com->protocol == CAPI_TCP_PROTOCOL)
			{
				ComTCP *this_ctcp;
				
				this_ctcp = (ComTCP *)curr->com;

				if(this_ctcp)
				{
					save_send_buffer = this_ctcp->send_buffer.buf;
					this_ctcp->send_buffer.buf = group->send_buffer + group->TCP_buffer_shift;
					
					if (curr->com->send_func){
						ret = curr->com->send_func(curr->com, msgsize, FALSE, type);
					}

					this_ctcp->send_buffer.buf = save_send_buffer ;
					
				}
			}
			else if (curr->com->protocol == CAPI_UDP_PROTOCOL)
			{
				ComIP *this_cudp;
				
				this_cudp = (ComIP *)curr->com;

				if(this_cudp)
				{
					save_send_buffer = this_cudp->send_buffer.buf;
					this_cudp->send_buffer.buf = group->send_buffer + group->UDP_buffer_shift;
					memcpy(this_cudp->send_buffer.buf,save_send_buffer,sizeof(ComAPIHeader));
					
					if (curr->com->send_func){
						ret = curr->com->send_func(curr->com, msgsize, FALSE, type);
					}

					this_cudp->send_buffer.buf = save_send_buffer ;
				}
			}
			else if(curr->com->protocol == CAPI_RUDP_PROTOCOL)
			{
				ComIP *this_cudp;
				
				this_cudp = (ComIP *)curr->com;

				if(this_cudp)
				{
					save_send_buffer = this_cudp->send_buffer.buf;
					this_cudp->send_buffer.buf = group->send_buffer + group->RUDP_buffer_shift;

					if (curr->com->send_func){
						ret = curr->com->send_func(curr->com, msgsize, FALSE, type);
					}

					this_cudp->send_buffer.buf = save_send_buffer ;
				}
			}
			/*else if
			(
				curr->com->protocol == CAPI_DPLAY_TCP_PROTOCOL || 
				curr->com->protocol == CAPI_DPLAY_MODEM_PROTOCOL ||
				curr->com->protocol == CAPI_DPLAY_SERIAL_PROTOCOL
			)
			{
				ret = ComDPLAYSendFromGroup(curr->com,msgsize,group->send_buffer +group->max_header);
			}*/
			else if(curr->com->protocol == CAPI_GROUP_PROTOCOL)  /* another group */
			{
				ComGROUP *this_group;
				this_group = (ComGROUP *) curr->com;
				if(this_group)
				{
					save_send_buffer = this_group->send_buffer;
					this_group->send_buffer = group->send_buffer; 

					if (curr->com->send_func) {
						ret = curr->com->send_func(curr->com, msgsize, FALSE, type);
					}

					this_group->send_buffer = save_send_buffer ;
				}
			}
			
			if (ret >= 0)
			{
				bytesSent += ret;
			}
			else
			{
				senderror = ret;
			}
		}
	}
	else
	{
		senderror = COMAPI_EMPTYGROUP;
	}
	leave_cs();
	if(senderror)
	{
		return senderror;
	}
	else
	{
		return bytesSent;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ComAPIHandle  ComAPICreateGroup(char *name_in, int BufferSize, ... )
{
	ComGROUP *group;
	va_list marker;
	LPVOID   vptr;
	int buffersize;
	int count=0;
	ComAPIHandle CH;
	unsigned int HostID=0;
	CAPIList
		*curr = 0;

	enter_cs ();
	
	buffersize = BufferSize; 
	va_start( marker, BufferSize );     /* Initialize variable arguments. */
	vptr = va_arg( marker, LPVOID);
	
	while( vptr != 0 )
	{
		count++;
		
		CH =  (ComAPIHandle)vptr;

		switch(CH->protocol)
		{
			case CAPI_UDP_PROTOCOL:
			{
				MonoPrint ("ComAPICreateGroup UDP\n");
				if (buffersize == 0)
				{
					buffersize = ((ComIP *)CH)->buffer_size;
				}
				else
				{
					buffersize = min(buffersize,(int)CH->query_func(CH,COMAPI_ACTUAL_BUFFER_SIZE));
				}

				if(HostID == 0)
				{
					CH->addr_func(CH,(char *)&HostID, 0);
				}
				
				break;
			}

			case CAPI_TCP_PROTOCOL:
			{	
				MonoPrint ("ComAPICreateGroup RUDP\n");
				if (buffersize == 0)
				{
					buffersize = ((ComTCP *)CH)->buffer_size;
				}
				else
				{
					buffersize = min(buffersize,(int)CH->query_func(CH,COMAPI_ACTUAL_BUFFER_SIZE));
				}

				if(HostID == 0)
				{
					CH->addr_func(CH,(char *)&HostID, 0);
				}
				
				break;
			}
				
			/* a trick -- use COMIP here/below since top of ComDPLAY struct is same as ComIP */
			case CAPI_DPLAY_SERIAL_PROTOCOL:
			case CAPI_DPLAY_MODEM_PROTOCOL:
			{
				MonoPrint ("ComAPICreateGroup Serial\n");
				if (buffersize == 0)
				{
					buffersize = ((ComIP *)CH)->buffer_size;  
				}
				else
				{
					buffersize = min(buffersize,(int)CH->query_func(CH,COMAPI_ACTUAL_BUFFER_SIZE));
				}

				if (HostID == 0)
				{
					CH->addr_func(CH,(char *)&HostID, 0);
				}
				
				break;
			}
				
			default:
			{
				break;
			}
		}
		
		vptr = va_arg( marker, LPVOID);
	}
	
	va_end( marker );  
	
	if (count == 0)
	{
		buffersize = BufferSize;
	}
	
	if(buffersize == 0 )
	{
		leave_cs ();

		return NULL;
	}
		
	MonoPrint ("ComAPICreateGroup CAPIListAppend GlobalGroupListHead\n");
	GlobalGroupListHead = CAPIListAppend(GlobalGroupListHead);

	if(!GlobalGroupListHead)
	{
		leave_cs ();

		return NULL;
	}
	
	/* allocate a new ComHandle struct */
	GlobalGroupListHead->com = (ComAPIHandle)malloc(sizeof(ComGROUP));
	memset(GlobalGroupListHead->com,0,sizeof(ComGROUP));
	GlobalGroupListHead->com->name = (char*)malloc (strlen (name_in) + 1);
	strcpy (GlobalGroupListHead->com->name, name_in);
	group = (ComGROUP *)  (GlobalGroupListHead->com);
	group->HostID = HostID;

#ifdef _DEBUG
	MonoPrint ("================================\n");
	MonoPrint ("GlobalGroupListHead\n");
	curr = GlobalGroupListHead;
	while (curr)
	{
		if (!F4IsBadReadPtrC(curr->com->name, 1)) // JB 010724 CTD
			MonoPrint ("  \"%s\"\n", curr->com->name);
		curr = curr->next;
	}
	MonoPrint ("================================\n");

	MonoPrint ("ComAPICreateGroup Created ComGroup \"%s\"\n", GlobalGroupListHead->com->name);
	MonoPrint ("ComAPICreateGroup Appended GGLH%08x CH:\"%s\" IP%08x\n", GlobalGroupListHead, GlobalGroupListHead->com->name, HostID);
#endif
	
	/* initialize header data */
	
	group->GroupHead  = NULL;
	
	group->apiheader.protocol          = CAPI_GROUP_PROTOCOL;
	
	group->apiheader.recv_func         = ComGROUPGet;
	group->apiheader.recv_buf_func     = ComGROUPRecvBufferGet;
	group->apiheader.addr_func         = ComGROUPAddress;
	group->apiheader.query_func        = ComGROUPQuery;
	group->apiheader.close_func        = ComGROUPClose;
	group->apiheader.send_buf_func     = ComGROUPSendBufferGet;
	group->apiheader.send_func         = ComGROUPSend;
	group->apiheader.sendX_func         = ComGROUPSendX;
	group->apiheader.get_timestamp_func = ComTCPGetTimeStamp;
	
	group->max_header =  max(sizeof(tcpHeader),sizeof(ComAPIHeader));
	group->buffer_size = buffersize + group->max_header;
	group->send_buffer = (char *)malloc(group->buffer_size);
	group->TCP_buffer_shift =  (char)(group->max_header - sizeof(tcpHeader));
	group->UDP_buffer_shift =  (char)(group->max_header - sizeof(ComAPIHeader));
	group->RUDP_buffer_shift = (char)group->max_header;
	group->DPLAY_buffer_shift = 0;

	leave_cs ();
	
	return (ComAPIHandle) group;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

unsigned long ComAPIGetNetHostBySocket(int Socket)
{
	int result,i;
	int retaddress=0;
	struct sockaddr_in Addr;
	SOCKET socket;
		
	socket = (SOCKET)Socket;
	
	if(socket == 0)
	{
		return 0;
	}
	
	i = sizeof(struct sockaddr);

	result = CAPI_getsockname(socket,(struct sockaddr FAR *)&Addr, &i);

	if(result == SOCKET_ERROR)
	{
		result = CAPI_WSAGetLastError();
		return -1 * result;
	}
	
	retaddress = Addr.sin_addr.s_addr;
	
	return retaddress;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

unsigned long ComAPIGetNetHostByHandle(ComAPIHandle c)
{
	ComTCP *ctcp;
	ComIP *cudp;
	unsigned long ip=0;
	
	if(c->protocol == CAPI_TCP_PROTOCOL)
	{
		ctcp = (ComTCP *)c;
		ip = ComAPIGetNetHostBySocket(ctcp->recv_sock);
	}
	else if(c->protocol == CAPI_TCP_PROTOCOL)
	{
		cudp = (ComIP *)c;
		ip = ComAPIGetNetHostBySocket(cudp->recv_sock);
	}
	
	return ip;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

unsigned long ComTCPGetTimeStamp(ComAPIHandle c)
{
	if(c)
	{
		ComTCP *ctcp = (ComTCP *)c;
		
		return ctcp->timestamp;
	}
	else
	{
		return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
