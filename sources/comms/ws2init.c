#pragma optimize( "", off )

/* ws2init.c - Copyright (c) Fri Dec 06 22:43:16 1996,  Spectrum HoloByte, Inc.  All Rights Reserved */
#include "capiopt.h"
#include <winsock.h>
#include <winbase.h>
#include <stdio.h>
#include "capi.h"
#include "capipriv.h"
//#define _CAPI_WSPROTOS_H_
#include "wsprotos.h"

#pragma warning(disable : 4706)

/*++
Routine Description:
    Calls WSAStartup, makes sure we have a good version of WinSock2

Arguments:      None.

Return Value:

    TRUE - WinSock 2 DLL successfully started up
    FALSE - Error starting up WinSock 2 DLL.

--*/
/* Global reference count for open connections */

int                              WS2Connections = 0;
static int                       CAPI_GetProcAddresses(HINSTANCE hWinSockDLL);
extern                           ComAPILastError;

HINSTANCE                        hWinSockDLL = 0;


/* WS function declarations */
WSFN_accept                      CAPI_accept = NULL;
WSFN_bind                        CAPI_bind = NULL;
WSFN_closesocket                 CAPI_closesocket = NULL;
WSFN_connect                     CAPI_connect = NULL;
WSFN_ioctlsocket                 CAPI_ioctlsocket = NULL;
WSFN_getsockopt                  CAPI_getsockopt = NULL;
WSFN_htonl                       CAPI_htonl = NULL;
WSFN_htons                       CAPI_htons = NULL;
WSFN_inet_addr                   CAPI_inet_addr = NULL;
WSFN_inet_ntoa                   CAPI_inet_ntoa = NULL;
WSFN_listen                      CAPI_listen = NULL;
WSFN_ntohl                       CAPI_ntohl = NULL;
WSFN_ntohs                       CAPI_ntohs = NULL;
WSFN_recv                        CAPI_recv = NULL;
WSFN_recvfrom                    CAPI_recvfrom = NULL;
WSFN_select                      CAPI_select = NULL;
WSFN_send                        CAPI_send = NULL;
WSFN_sendto                      CAPI_sendto = NULL;
WSFN_setsockopt                  CAPI_setsockopt = NULL;
WSFN_shutdown                    CAPI_shutdown = NULL;
WSFN_socket                      CAPI_socket = NULL;
WSFN_gethostbyaddr               CAPI_gethostbyaddr = NULL;
WSFN_gethostbyname               CAPI_gethostbyname = NULL; 
WSFN_gethostname                 CAPI_gethostname = NULL;
WSFN_getsockname                 CAPI_getsockname = NULL; 
WSFN_WSAStartup                  CAPI_WSAStartup = NULL;
WSFN_WSACleanup                  CAPI_WSACleanup = NULL;
WSFN_WSASetLastError             CAPI_WSASetLastError = NULL;
WSFN_WSAGetLastError             CAPI_WSAGetLastError = NULL; 

int InitWS2(WSADATA *wsaData)
{
  int wsaStatus;
  int buflen;
  HINSTANCE  hWinSockDLL = 0;
  char  dllname[20];
  int Major, Minor;

   strcpy(dllname,"WSOCK32.DLL");
   Major = 1;
   Minor = 1;

if(!WS2Connections)  /* No successfull connection yet? */
{
 buflen = SearchPath(NULL, dllname, NULL,0,NULL,NULL);	
 if(buflen == 0 )
 {
    ComAPILastError = COMAPI_WINSOCKDLL_ERROR;
    return 0;
 }

 #ifdef LOAD_DLLS
 hWinSockDLL = LoadLibrary(dllname);

 if (hWinSockDLL == NULL) {
    ComAPILastError = COMAPI_WINSOCKDLL_ERROR;
    return 0;
 }
 #endif
 
 if(!CAPI_GetProcAddresses(hWinSockDLL)) {
#ifdef LOAD_DLLS
     FreeLibrary(hWinSockDLL);
     ComAPILastError = COMAPI_WINSOCKDLL_ERROR;
#endif
     return 0;
 }
 



  if(wsaStatus=CAPI_WSAStartup(MAKEWORD(Major,Minor), wsaData))
    {
      /*
        MessageBox(GlobalFrameWindow,
        "Could not find high enough version of WinSock",
        "Error", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        */
      return 0;
    }
  else
    {
      /* Now confirm that the WinSock 2 DLL supports the exact version */
      /* we want. If not, make sure to call WSACleanup(). */
      if (LOBYTE(wsaData->wVersion) != Major ||
          HIBYTE(wsaData->wVersion) != Minor)
        {
          /*
            MessageBox(GlobalFrameWindow,
            "Could not find the correct version of WinSock",
            "Error",  MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            */
          CAPI_WSACleanup();
          return 0;
        }

    }
}

/* if we get here , either we just need to increment counter
   or we execute the first successful WSAStartup */

WS2Connections++;


return 1;

}


static int CAPI_GetProcAddresses(HINSTANCE hWinSockDLL)
{

#ifdef LOAD_DLLS

CAPI_accept            = (WSFN_accept) GetProcAddress(hWinSockDLL,"accept");
if (CAPI_accept == NULL ) return 0;
CAPI_bind              = (WSFN_bind)GetProcAddress(hWinSockDLL,"bind");
if (CAPI_bind == NULL )  return 0;

CAPI_closesocket       = (WSFN_closesocket)GetProcAddress(hWinSockDLL,"closesocket");
if (CAPI_closesocket == NULL )  return 0;

CAPI_connect           = (WSFN_connect)GetProcAddress(hWinSockDLL,"connect");;
if (CAPI_connect == NULL )  return 0;

CAPI_ioctlsocket       = (WSFN_ioctlsocket)GetProcAddress(hWinSockDLL,"ioctlsocket");
if (CAPI_ioctlsocket == NULL )  return 0;

CAPI_getsockopt        = (WSFN_getsockopt)GetProcAddress(hWinSockDLL,"getsockopt");
if (CAPI_getsockopt == NULL )  return 0;

CAPI_htonl             = (WSFN_htonl)GetProcAddress(hWinSockDLL,"htonl");
if (CAPI_htonl == NULL )  return 0;

CAPI_htons             = (WSFN_htons)GetProcAddress(hWinSockDLL,"htons");
if (CAPI_htons == NULL )  return 0;

CAPI_inet_addr         = (WSFN_inet_addr)GetProcAddress(hWinSockDLL,"inet_addr");
if (CAPI_inet_addr == NULL )  return 0;

CAPI_inet_ntoa         = (WSFN_inet_ntoa)GetProcAddress(hWinSockDLL,"inet_ntoa");
if (CAPI_inet_ntoa == NULL )  return 0;

CAPI_listen            = (WSFN_listen)GetProcAddress(hWinSockDLL,"listen");
if (CAPI_listen == NULL )  return 0;

CAPI_ntohl             = (WSFN_ntohl)GetProcAddress(hWinSockDLL,"ntohl");
if (CAPI_ntohl == NULL )  return 0;

CAPI_ntohs             = (WSFN_ntohs)GetProcAddress(hWinSockDLL,"ntohs");
if (CAPI_ntohs == NULL )  return 0;

CAPI_recv              = (WSFN_recv)GetProcAddress(hWinSockDLL,"recv");
if (CAPI_recv == NULL )  return 0;

CAPI_recvfrom          = (WSFN_recvfrom)GetProcAddress(hWinSockDLL,"recvfrom");
if (CAPI_recvfrom == NULL )  return 0;

CAPI_select            = (WSFN_select)GetProcAddress(hWinSockDLL,"select");
if (CAPI_select == NULL )  return 0;

CAPI_send              = (WSFN_send)GetProcAddress(hWinSockDLL,"send");
if (CAPI_send == NULL )  return 0;

CAPI_sendto            = (WSFN_sendto)GetProcAddress(hWinSockDLL,"sendto");
if (CAPI_sendto == NULL )  return 0;

CAPI_setsockopt        = (WSFN_setsockopt)GetProcAddress(hWinSockDLL,"setsockopt");
if (CAPI_setsockopt == NULL )  return 0;

CAPI_shutdown          = (WSFN_shutdown)GetProcAddress(hWinSockDLL,"shutdown");
if (CAPI_shutdown == NULL )  return 0;

CAPI_socket            = (WSFN_socket)GetProcAddress(hWinSockDLL,"socket");
if (CAPI_socket == NULL )  return 0;

CAPI_gethostbyaddr     = (WSFN_gethostbyaddr)GetProcAddress(hWinSockDLL,"gethostbyaddr");
if (CAPI_gethostbyaddr == NULL )  return 0;

CAPI_gethostbyname     = (WSFN_gethostbyname)GetProcAddress(hWinSockDLL,"gethostbyname"); 
if (CAPI_gethostbyname == NULL )  return 0;

CAPI_gethostname       = (WSFN_gethostname)GetProcAddress(hWinSockDLL,"gethostname");
if (CAPI_gethostname == NULL )  return 0;

CAPI_getsockname       = (WSFN_getsockname)GetProcAddress(hWinSockDLL,"getsockname");
if (CAPI_getsockname == NULL )  return 0;


CAPI_WSAStartup        = (WSFN_WSAStartup)GetProcAddress(hWinSockDLL,"WSAStartup");
if (CAPI_WSAStartup == NULL )  return 0;

CAPI_WSACleanup        = (WSFN_WSACleanup )GetProcAddress(hWinSockDLL,"WSACleanup");
if (CAPI_WSACleanup == NULL )  return 0;

CAPI_WSASetLastError   = (WSFN_WSASetLastError)GetProcAddress(hWinSockDLL,"WSASetLastError");
if (CAPI_WSASetLastError == NULL )  return 0;

CAPI_WSAGetLastError   = (WSFN_WSAGetLastError)GetProcAddress(hWinSockDLL,"WSAGetLastError"); 
if (CAPI_WSAGetLastError == NULL )  return 0;



#else
CAPI_accept            = (WSFN_accept)accept;
CAPI_bind              = (WSFN_bind)bind;

CAPI_closesocket       = (WSFN_closesocket)closesocket;

CAPI_connect           = (WSFN_connect)connect;;

CAPI_ioctlsocket       = (WSFN_ioctlsocket)ioctlsocket;

CAPI_getsockopt        = (WSFN_getsockopt)getsockopt;

CAPI_htonl             = (WSFN_htonl)htonl;

CAPI_htons             = (WSFN_htons)htons;

CAPI_inet_addr         = (WSFN_inet_addr)inet_addr;

CAPI_inet_ntoa         = (WSFN_inet_ntoa)inet_ntoa;

CAPI_listen            = (WSFN_listen)listen;

CAPI_ntohl             = (WSFN_ntohl)ntohl;

CAPI_ntohs             = (WSFN_ntohs)ntohs;

CAPI_recv              = (WSFN_recv)recv;

CAPI_recvfrom          = (WSFN_recvfrom)recvfrom;

CAPI_select            = (WSFN_select)select;

CAPI_send              = (WSFN_send)send;

CAPI_sendto            = (WSFN_sendto)sendto;

CAPI_setsockopt        = (WSFN_setsockopt)setsockopt;

CAPI_shutdown          = (WSFN_shutdown)shutdown;

CAPI_socket            = (WSFN_socket)socket;

CAPI_gethostbyaddr     = (WSFN_gethostbyaddr)gethostbyaddr;

CAPI_gethostbyname     = (WSFN_gethostbyname)gethostbyname; 

CAPI_gethostname       = (WSFN_gethostname)gethostname;

CAPI_getsockname       = (WSFN_getsockname)getsockname;

CAPI_WSAStartup        = (WSFN_WSAStartup)WSAStartup;

CAPI_WSACleanup        = (WSFN_WSACleanup )WSACleanup;

CAPI_WSASetLastError   = (WSFN_WSASetLastError)WSASetLastError;

CAPI_WSAGetLastError   = (WSFN_WSAGetLastError)WSAGetLastError; 

#endif

return 1;

}
