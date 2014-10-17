#ifndef WSPROTOS_H
#define WSPROTOS_H

//#include <winsock.h>

typedef SOCKET (PASCAL FAR *wsfn_accept)(SOCKET s,
                                         struct sockaddr FAR *addr,
                                         int FAR *addrlen);

typedef int (PASCAL FAR *wsfn_bind)(SOCKET s,
									const struct sockaddr FAR *addr,
			                        int namelen);

typedef int (PASCAL FAR *wsfn_closesocket)(SOCKET s);

typedef int (PASCAL FAR *wsfn_connect)(SOCKET s,
									   const struct sockaddr FAR *name,
									   int namelen);

typedef int (PASCAL FAR *wsfn_ioctlsocket)(SOCKET s,
						                   long cmd,
										   u_long FAR *argp);

typedef int (PASCAL FAR *wsfn_getsockopt)(SOCKET s,
						   			      int level,
										  int optname,
                                          char FAR * optval,
										  int FAR *optlen);

typedef int (PASCAL FAR *wsfn_getsockname)(SOCKET s,
                                           struct sockaddr FAR *name,
                                           int FAR * namelen);

typedef u_long (PASCAL FAR *wsfn_htonl)(u_long hostlong);

typedef u_short (PASCAL FAR *wsfn_htons)(u_short hostshort);

typedef unsigned long (PASCAL FAR *wsfn_inet_addr)(const char FAR *cp);

typedef char FAR* (PASCAL FAR *wsfn_inet_ntoa)(struct in_addr in);

typedef int (PASCAL FAR *wsfn_listen)(SOCKET s,
									  int backlog);

typedef u_long (PASCAL FAR *wsfn_ntohl)(u_long netlong);

typedef u_short (PASCAL FAR *wsfn_ntohs)(u_short netshort);

typedef int (PASCAL FAR *wsfn_recv)(SOCKET s,
									char FAR *buf,
									int len,
									int flags);

typedef int (PASCAL FAR *wsfn_recvfrom)(SOCKET s,
										char FAR * buf,
										int len,
										int flags,
                                        struct sockaddr FAR *from,
									    int FAR * fromlen);

typedef int (PASCAL FAR *wsfn_select)(int nfds,
									  fd_set FAR *readfds,
									  fd_set FAR *writefds,
                                      fd_set FAR *exceptfds,
									  const struct timeval FAR *timeout);

typedef int (PASCAL FAR *wsfn_send)(SOCKET s,
									const char FAR * buf,
									int len, int flags);

typedef int (PASCAL FAR *wsfn_sendto)(SOCKET s,
									  const char FAR *buf,
									  int len, int flags,
                                      const struct sockaddr FAR *to,
									  int tolen);

typedef int (PASCAL FAR *wsfn_setsockopt)(SOCKET s,
										  int level,
										  int optname,
                                          const char FAR *optval,
										  int optlen);

typedef int (PASCAL FAR *wsfn_shutdown)(SOCKET s,
										int how);

typedef SOCKET (PASCAL FAR *wsfn_socket)(int af,
										 int type,
										 int protocol);

/* Database function prototypes */

typedef struct hostent FAR* (PASCAL FAR *wsfn_gethostbyaddr)(
	                                                      const char FAR *addr,
                                                          int len,
														  int type);

typedef struct hostent FAR* (PASCAL FAR *wsfn_gethostbyname)(
	                                                     const char FAR *name);

typedef int (PASCAL FAR *wsfn_gethostname)(char FAR* name,
										   int namelen);


/* Microsoft Windows Extension function prototypes */

typedef int (PASCAL FAR *wsfn_WSAStartup)(WORD wVersionRequired,
										  LPWSADATA lpWSAData);

typedef int (PASCAL FAR *wsfn_WSACleanup)(void);

typedef void (PASCAL FAR *wsfn_WSASetLastError)(int iError);

typedef int (PASCAL FAR *wsfn_WSAGetLastError)(void);


#ifdef __cplusplus
    extern "C" {
#endif

		extern wsfn_accept capi_accept;
		extern wsfn_bind capi_bind;
		extern wsfn_closesocket capi_closesocket;
		extern wsfn_connect capi_connect;
		extern wsfn_ioctlsocket capi_ioctlsocket;
		extern wsfn_getsockopt capi_getsockopt;
		extern wsfn_htonl capi_htonl;
		extern wsfn_htons capi_htons;
		extern wsfn_inet_addr capi_inet_addr;
		extern wsfn_inet_ntoa capi_inet_ntoa;
		extern wsfn_listen capi_listen;
		extern wsfn_ntohl capi_ntohl;
		extern wsfn_ntohs capi_ntohs;
		extern wsfn_recv capi_recv;
		extern wsfn_recvfrom capi_recvfrom;
		extern wsfn_select capi_select;
		extern wsfn_send capi_send;
		extern wsfn_sendto capi_sendto;
		extern wsfn_setsockopt capi_setsockopt;
		extern wsfn_shutdown capi_shutdown;
		extern wsfn_socket capi_socket;
		extern wsfn_gethostbyaddr capi_gethostbyaddr;
		extern wsfn_gethostbyname capi_gethostbyname;
		extern wsfn_gethostname capi_gethostname;
		extern wsfn_getsockname capi_getsockname;
		extern wsfn_WSAStartup capi_WSAStartup;
		extern wsfn_WSACleanup capi_WSACleanup;
		extern wsfn_WSASetLastError capi_WSASetLastError;
		extern wsfn_WSAGetLastError capi_WSAGetLastError;

#ifdef __cplusplus
    }
#endif

#endif // WSPROTOS_H