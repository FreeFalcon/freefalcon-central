#ifndef _CAPI_WSPROTOS_H_
#define _CAPI_WSPROTOS_H_

#include <winsock.h>

typedef SOCKET (PASCAL FAR  *WSFN_accept) (SOCKET s, struct sockaddr FAR *addr,
                          int FAR *addrlen);

typedef int (PASCAL FAR *WSFN_bind) (SOCKET s, const struct sockaddr FAR *addr, int namelen);

typedef int (PASCAL FAR  *WSFN_closesocket) (SOCKET s);

typedef int (PASCAL FAR *WSFN_connect) (SOCKET s, const struct sockaddr FAR *name, int namelen);

typedef int (PASCAL FAR  *WSFN_ioctlsocket) (SOCKET s, long cmd, u_long FAR *argp);


typedef int (PASCAL FAR  *WSFN_getsockopt) (SOCKET s, int level, int optname,
                           char FAR * optval, int FAR *optlen);

typedef int (PASCAL FAR *WSFN_getsockname)(SOCKET s, struct sockaddr FAR *name,
                            int FAR * namelen);

typedef u_long (PASCAL FAR  *WSFN_htonl) (u_long hostlong);

typedef u_short (PASCAL FAR  *WSFN_htons) (u_short hostshort);

typedef unsigned long (PASCAL FAR  *WSFN_inet_addr) (const char FAR * cp);

typedef char FAR * (PASCAL FAR  *WSFN_inet_ntoa) (struct in_addr in);

typedef int (PASCAL FAR  *WSFN_listen) (SOCKET s, int backlog);

typedef u_long (PASCAL FAR  *WSFN_ntohl) (u_long netlong);

typedef u_short (PASCAL FAR  *WSFN_ntohs) (u_short netshort);

typedef int (PASCAL FAR  *WSFN_recv) (SOCKET s, char FAR * buf, int len, int flags);

typedef int (PASCAL FAR  *WSFN_recvfrom) (SOCKET s, char FAR * buf, int len, int flags,
                         struct sockaddr FAR *from, int FAR * fromlen);

typedef int (PASCAL FAR *WSFN_select) (int nfds, fd_set FAR *readfds, fd_set FAR *writefds,
                       fd_set FAR *exceptfds, const struct timeval FAR *timeout);

typedef int (PASCAL FAR  *WSFN_send) (SOCKET s, const char FAR * buf, int len, int flags);

typedef int (PASCAL FAR  *WSFN_sendto) (SOCKET s, const char FAR * buf, int len, int flags,
                       const struct sockaddr FAR *to, int tolen);

typedef int (PASCAL FAR  *WSFN_setsockopt) (SOCKET s, int level, int optname,
                           const char FAR * optval, int optlen);

typedef int (PASCAL FAR  *WSFN_shutdown) (SOCKET s, int how);

typedef SOCKET (PASCAL FAR *WSFN_socket) (int af, int type, int protocol);

/* Database function prototypes */

typedef struct hostent FAR * (PASCAL FAR *WSFN_gethostbyaddr)(const char FAR * addr,
                                              int len, int type);

typedef struct hostent FAR * (PASCAL FAR  *WSFN_gethostbyname)(const char FAR * name);

typedef int (PASCAL FAR  *WSFN_gethostname) (char FAR * name, int namelen);


/* Microsoft Windows Extension function prototypes */

typedef int (PASCAL FAR  *WSFN_WSAStartup)(WORD wVersionRequired, LPWSADATA lpWSAData);

typedef int (PASCAL FAR *WSFN_WSACleanup)(void);

typedef void (PASCAL FAR  *WSFN_WSASetLastError)(int iError);

typedef int (PASCAL FAR *WSFN_WSAGetLastError)(void);


//#ifndef _CAPI_WSPROTOS_H_
//#define _CAPI_WSPROTOS_H_
#ifdef __cplusplus
extern "C" {
#endif

extern  WSFN_accept           CAPI_accept;
extern  WSFN_bind             CAPI_bind;
extern  WSFN_closesocket      CAPI_closesocket;
extern  WSFN_connect          CAPI_connect;
extern  WSFN_ioctlsocket      CAPI_ioctlsocket;
extern  WSFN_getsockopt       CAPI_getsockopt;
extern  WSFN_htonl            CAPI_htonl;
extern  WSFN_htons            CAPI_htons;
extern  WSFN_inet_addr        CAPI_inet_addr;
extern  WSFN_inet_ntoa        CAPI_inet_ntoa;
extern  WSFN_listen           CAPI_listen;
extern  WSFN_ntohl            CAPI_ntohl;
extern  WSFN_ntohs            CAPI_ntohs;
extern  WSFN_recv             CAPI_recv;
extern  WSFN_recvfrom         CAPI_recvfrom;
extern  WSFN_select           CAPI_select;
extern  WSFN_send             CAPI_send;
extern  WSFN_sendto           CAPI_sendto;
extern  WSFN_setsockopt       CAPI_setsockopt;
extern  WSFN_shutdown         CAPI_shutdown;
extern  WSFN_socket           CAPI_socket;
extern  WSFN_gethostbyaddr    CAPI_gethostbyaddr;
extern  WSFN_gethostbyname    CAPI_gethostbyname; 
extern  WSFN_gethostname      CAPI_gethostname;
extern  WSFN_getsockname      CAPI_getsockname;
extern  WSFN_WSAStartup       CAPI_WSAStartup;
extern  WSFN_WSACleanup       CAPI_WSACleanup;
extern  WSFN_WSASetLastError  CAPI_WSASetLastError;
extern  WSFN_WSAGetLastError  CAPI_WSAGetLastError; 

#ifdef __cplusplus
}
#endif

#endif
