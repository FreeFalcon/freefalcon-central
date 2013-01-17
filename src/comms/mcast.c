/* multicst.c - Copyright (c) Fri Dec 06 17:27:16 1996,  Spectrum HoloByte, Inc.  All Rights Reserved */

#pragma optimize( "", off ) // JB 010718

#include <stdio.h>
#include <winsock2.h>
#include <assert.h>
#include "capi.h"
#include "capipriv.h"
#include "ws2init.h"
#include "fndproto.h"
#include "protofilt.h"


#define CAPI_UDP_PORT 2934

static struct sockaddr_in comSendAddr, comRecvAddr;

/* forward function declarations */
void ComIPMulticastClose(ComAPIHandle c);
int ComIPMulticastSend(ComAPIHandle c, int msgsize);
int ComIPMulticastGet(ComAPIHandle c);

int ComIPHostIDGet(ComAPIHandle c, char *buf);
char *ComIPSendBufferGet(ComAPIHandle c);
char *ComIPRecvBufferGet(ComAPIHandle c);

/* begin a comms session */
ComAPIHandle ComIPMulticastOpen(int buffersize, char *gamename, int mc_scope)
{
  LPWSAPROTOCOL_INFO protocols = 0;
  LPWSAPROTOCOL_INFO mc_protocol = 0;
  int numProtocols = 0;
  ComIP *c;
  int i, err;
  int trueValue=1;
  int falseValue=0;

  c = (ComIP*)malloc(sizeof(ComIP));

  if (InitWS2(&c->wsaData) == 0 ||
      FindProtocols(&protocols, &numProtocols) == 0) {
    free(c);
    return 0;
  }

  /* verify that we have a multi-cast protocol */
/*   for (i = 0; i < numProtocols && mc_protocol == 0; i++) { */
  for (i = 0; i < numProtocols; i++) {
    if (!mc_protocol &&
	protocols[i].dwServiceFlags1 & XP1_SUPPORT_MULTIPOINT &&
	!(protocols[i].dwServiceFlags1 & XP1_MULTIPOINT_CONTROL_PLANE) &&
	!(protocols[i].dwServiceFlags1 & XP1_MULTIPOINT_DATA_PLANE)) {
      mc_protocol = &protocols[i];
#ifdef DEBUG_COMMS
      printf("Found a suitable protocol:\n");
      printf("  iProtocol = %d\n", mc_protocol->iProtocol);
      printf("  iSocketType = %d\n", mc_protocol->iSocketType);
      printf("  iAddressFamily = %d\n", mc_protocol->iAddressFamily);
      printf("  flags = 0x%x\n", mc_protocol->dwServiceFlags1);
      printf("  msgsize = %d\n", mc_protocol->dwMessageSize);
      printf("  name = '%s'\n", mc_protocol->szProtocol);
    } else {
      printf("Unsuitable protocol:\n");
      printf("  iProtocol = %d\n", protocols[i].iProtocol);
      printf("  flags = 0x%x\n", protocols[i].dwServiceFlags1);
      printf("  msgsize = %d\n", protocols[i].dwMessageSize);
      printf("  name = '%s'\n", protocols[i].szProtocol);
#endif
    }
  }

  if (mc_protocol == 0) {
#ifdef DEBUG_COMMS
    printf("ComMulticastOpen: Found no suitable protocols\n");
#endif
    free(protocols);
    free(c);
    return 0;
  }

  /* initialize header data */
  c->apiheader.protocol = CAPI_IP_MULTICAST_PROTOCOL;
  c->apiheader.send_func = ComIPMulticastSend;
  c->apiheader.recv_func = ComIPMulticastGet;
  c->apiheader.send_buf_func = ComIPSendBufferGet;
  c->apiheader.recv_buf_func = ComIPRecvBufferGet;
  c->apiheader.addr_func = ComIPHostIDGet;
  c->apiheader.close_func = ComIPMulticastClose;

  c->buffer_size = sizeof(ComAPIHeader) + buffersize;

  c->send_buffer.buf = (char *)malloc(c->buffer_size);
  ComIPHostIDGet(&c->apiheader, c->send_buffer.buf);
#ifdef DEBUG_COMMS
  printf("ComAPIOpen -- got id 0x%x\n", 
	    ((ComAPIHeader *)c->send_buffer.buf)->sender);
#endif
  strncpy(((ComAPIHeader *)c->send_buffer.buf)->gamename, gamename,
		GAME_NAME_LENGTH);
  c->recv_buffer.buf = (char *)malloc(c->buffer_size);

  /* Incoming... */
#if 1
  c->recv_sock = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, 0, 0,
#else
  c->recv_sock = WSASocket(FROM_PROTOCOL_INFO,
      			   FROM_PROTOCOL_INFO,
			   FROM_PROTOCOL_INFO,
                           mc_protocol, 0,
#endif
			WSA_FLAG_MULTIPOINT_C_LEAF|WSA_FLAG_MULTIPOINT_D_LEAF);

  if(c->recv_sock == INVALID_SOCKET)
    {
      err = WSAGetLastError();
      free(protocols);
      free(c);
      return 0;
    }

  memset ((char*)&comRecvAddr, 0, sizeof(comRecvAddr));
/*   comRecvAddr.sin_family       = mc_protocol->iAddressFamily; */
  comRecvAddr.sin_family       = AF_INET;
  comRecvAddr.sin_addr.s_addr  = htonl(0x9d000001);
  comRecvAddr.sin_addr.s_addr  = htonl(0x0);
  comRecvAddr.sin_port         = htons(CAPI_UDP_PORT);

#ifdef DEBUG_COMMS
  printf("binding (recv) socket #%d\n", c->recv_sock);
#endif
  if(err=bind(c->recv_sock, (struct sockaddr*)&comRecvAddr,sizeof(comRecvAddr)))
    {
      err = WSAGetLastError();
#ifdef DEBUG_COMMS
      printf("bind (recv) error #%d\n", err);
#endif
      free(protocols);
      free(c);
      return 0;
    }

/*   WSAIoctl(c->recv_sock, FIONBIO, &trueValue, sizeof(trueValue), 0, 0, 0, 0, 0); */
  WSAIoctl(c->recv_sock, SIO_MULTIPOINT_LOOPBACK, &falseValue, sizeof(falseValue),
      	0, 0, 0, 0, 0);
  WSAIoctl(c->recv_sock, SIO_MULTICAST_SCOPE, &mc_scope, sizeof(mc_scope),
      	0, 0, 0, 0, 0);

  c->send_sock = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, 0, 0,
			WSA_FLAG_MULTIPOINT_C_LEAF|WSA_FLAG_MULTIPOINT_D_LEAF);
  if(c->send_sock == INVALID_SOCKET)
    {
      err = WSAGetLastError();
      free(protocols);
      free(c);
      return 0;
    }

  /* Outgoing... */
  memset ((char*)&comSendAddr, 0, sizeof(comSendAddr));
  comSendAddr.sin_family       = AF_INET;
/*   comSendAddr.sin_addr.s_addr  = htonl(INADDR_ANY); */
  comSendAddr.sin_addr.s_addr  = htonl(0xe0000001);
  comSendAddr.sin_port         = htons(CAPI_UDP_PORT);

/*   setsockopt(c->send_sock, SOL_SOCKET, SO_BROADCAST, */
/* 	    	(char *)&trueValue, sizeof(int) );   */

  c->recv_sock = WSAJoinLeaf(c->recv_sock,
      (struct sockaddr *)&comRecvAddr, sizeof(comRecvAddr),
      0, 0, 0, 0, JL_BOTH);
/*       0, 0, 0, 0, JL_RECEIVER_ONLY); */

  if (c->recv_sock == INVALID_SOCKET)
    {
      err = WSAGetLastError();
#ifdef DEBUG_COMMS
      printf("WSAJoinLeaf (recv) error #%d\n", err);
#endif
      free(protocols);
      free(c);
      return 0;
    }
  WSAIoctl(c->recv_sock, FIONBIO, &trueValue, sizeof(trueValue), 0, 0, 0, 0, 0);

  free(protocols);
  return (ComAPIHandle)c;
}


/* end a comms session */
void ComIPMulticastClose(ComAPIHandle c)
{
  int sockerror;

  if(c)
    {
      ComIP *cudp = (ComIP *)c;

      if(sockerror = closesocket(cudp->recv_sock))
        {
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
      WSACleanup();
      free(cudp->recv_buffer.buf);
      free(cudp->send_buffer.buf);
      free(c);
    }
}

/* send data from a comms session */
int ComIPMulticastSend(ComAPIHandle c, int msgsize, int oob)
{
  if(c)
    {
      ComIP *cudp = (ComIP *)c;
      int senderror;
      int bytesSent;

      cudp->send_buffer.len = msgsize + sizeof(ComAPIHeader);

#ifdef DEBUG_COMMS
      printf("ComAPISend -- sending message\n");
#endif

      if(senderror = WSASendTo(cudp->send_sock,
			       &cudp->send_buffer, 1,
			       &bytesSent,
			       0,
			       (struct sockaddr *)&comSendAddr,
			       sizeof(comSendAddr),
			       0,
			       0))
        {
	  senderror = WSAGetLastError();
          switch(senderror)
            {
            default :
#ifdef DEBUG_COMMS
	      printf("ComAPISend error #%d (sock #%d)\n", senderror, cudp->send_sock);
#endif
              return 0;
            }
        }

      return 1;
    }
  else
    {
      return 0;
    }
}

/* recive data from a comms session */
int ComIPMulticastGet(ComAPIHandle c)
{
  if(c)
    {
      ComIP *cudp = (ComIP *)c;
      int recverror;
      int bytesRecvd;
      int flags = 0;

      cudp->recv_buffer.len = cudp->buffer_size;

#if 1
      if(SOCKET_ERROR == WSARecv(cudp->recv_sock,
                             &cudp->recv_buffer, 1,
                             &bytesRecvd,
                             &flags,
                             0,
                             0))
#else
      if (bytesRecvd = recv(cudp->recv_sock, buf, *bufsize, 0) < 0)
#endif
        {
	  recverror = WSAGetLastError();
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
              break;
            }
        }
      if (bytesRecvd > 0)  {
	if (((ComAPIHeader *)cudp->recv_buffer.buf)->sender ==
	    ((ComAPIHeader *)cudp->send_buffer.buf)->sender) {
#ifdef DEBUG_COMMS
	  printf("ComAPIReceive -- got our own message id 0x%x\n",
	    ((ComAPIHeader *)cudp->send_buffer.buf)->sender);
#endif
	  return 0;
	}
	if (strncmp(((ComAPIHeader *)cudp->recv_buffer.buf)->gamename,
	    ((ComAPIHeader *)cudp->send_buffer.buf)->gamename, GAME_NAME_LENGTH)) {
#ifdef DEBUG_COMMS
	  ((ComAPIHeader *)cudp->recv_buffer.buf)->gamename[GAME_NAME_LENGTH] = 0;
	  printf("ComAPIReceive -- got a message from another game '%s'\n",
	      	((ComAPIHeader *)cudp->recv_buffer.buf)->gamename);
#endif
	  return 0;
	}
#ifdef DEBUG_COMMS
	printf("ComAPIReceive -- got message -- bytesRecvd = %d\n", bytesRecvd);
#endif
	return 1;
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

