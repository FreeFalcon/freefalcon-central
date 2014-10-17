// SYSTEM INCLUDES
//#include <WinSock.h>
//#include <WinBase.h>
//#include <StdIo.h>
// END OF SYSTEM INCLUDES


// SIM INCLUDES
//#include "CapiOpt.h"
//#include "capi.h"
#include "CapiPriv.h"
#include "WsProtos.h"
// END OF SIM INCLUDES



// PREPROCESSOR DIRECTIVES
//#define _CAPI_WSPROTOS_H_
// END OF PREPROCESSOR DIRECTIVES



/*++
Routine Description:
    Calls WSAStartup, makes sure we have a good version of WinSock2

Arguments:      None.

Return Value:

    TRUE - WinSock 2 DLL successfully started up
    FALSE - Error starting up WinSock 2 DLL.

--*/
/* Global reference count for open connections */



// GLOBAL VARIABLES
int windows_sockets_connections = 0;
extern com_api_last_error;
HINSTANCE windows_sockets_dll = 0;
// END OF GLOBAL VARIABLES



// FUNCTION DECLARATIONS
static int capi_get_process_address(HINSTANCE windows_sockets_dll);

wsfn_accept capi_accept = NULL;
wsfn_bind capi_bind = NULL;
wsfn_closesocket capi_closesocket = NULL;
wsfn_connect capi_connect = NULL;
wsfn_ioctlsocket capi_ioctlsocket = NULL;
wsfn_getsockopt capi_getsockopt = NULL;
wsfn_htonl capi_htonl = NULL;
wsfn_htons capi_htons = NULL;
wsfn_inet_addr capi_inet_addr = NULL;
wsfn_inet_ntoa capi_inet_ntoa = NULL;
wsfn_listen capi_listen = NULL;
wsfn_ntohl capi_ntohl = NULL;
wsfn_ntohs capi_ntohs = NULL;
wsfn_recv capi_recv = NULL;
wsfn_recvfrom capi_recvfrom = NULL;
wsfn_select capi_select = NULL;
wsfn_send capi_send = NULL;
wsfn_sendto capi_sendto = NULL;
wsfn_setsockopt capi_setsockopt = NULL;
wsfn_shutdown capi_shutdown = NULL;
wsfn_socket capi_socket = NULL;
wsfn_gethostbyaddr capi_gethostbyaddr = NULL;
wsfn_gethostbyname capi_gethostbyname = NULL;
wsfn_gethostname capi_gethostname = NULL;
wsfn_getsockname capi_getsockname = NULL;
wsfn_WSAStartup capi_WSAStartup = NULL;
wsfn_WSACleanup capi_WSACleanup = NULL;
wsfn_WSASetLastError capi_WSASetLastError = NULL;
wsfn_WSAGetLastError capi_WSAGetLastError = NULL;
// END OF FUNCTION DECLARATIONS



// FUNCTION DEFINITIONS
int initialize_windows_sockets(WSADATA* windows_socket_data)
{
	const char* DLL_NAME = "WSOCK32.DLL";
	const int MAJOR_VERSION = 1;
	const int MINOR_VERSION = 1;

	int buffer_length;

	int wsaStatus;
    HINSTANCE  windows_sockets_dll = 0;


    if (!windows_sockets_connections) // No successful connection yet? 
    {
        buffer_length = SearchPath(NULL, DLL_NAME, NULL, 0, NULL, NULL);

        if (buffer_length == 0)
        {
            com_api_last_error = COMAPI_WINSOCKDLL_ERROR;
            return 0;
        }

#ifdef LOAD_DLLS
        windows_sockets_dll = LoadLibrary(DLL_NAME);

        if (windows_sockets_dll == NULL)
        {
            com_api_last_error = COMAPI_WINSOCKDLL_ERROR;
            return 0;
        }

#endif

        if (!capi_get_process_address(windows_sockets_dll))
        {
#ifdef LOAD_DLLS
            FreeLibrary(windows_sockets_dll);
            com_api_last_error = COMAPI_WINSOCKDLL_ERROR;
#endif
            return 0;
        }




        if (wsaStatus = capi_WSAStartup(MAKEWORD(MAJOR_VERSION, MINOR_VERSION), windows_socket_data))
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
            if (LOBYTE(windows_socket_data->wVersion) != MAJOR_VERSION ||
                HIBYTE(windows_socket_data->wVersion) != MINOR_VERSION)
            {
                /*
                  MessageBox(GlobalFrameWindow,
                  "Could not find the correct version of WinSock",
                  "Error",  MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
                  */
                capi_WSACleanup();
                return 0;
            }

        }
    }

    /* if we get here , either we just need to increment counter
       or we execute the first successful WSAStartup */

    windows_sockets_connections++;


    return 1;

}


static int capi_get_process_address(HINSTANCE windows_sockets_dll)
{

#ifdef LOAD_DLLS

    capi_accept            = (wsfn_accept) GetProcAddress(windows_sockets_dll, "accept");

    if (capi_accept == NULL) return 0;

    capi_bind              = (wsfn_bind)GetProcAddress(windows_sockets_dll, "bind");

    if (capi_bind == NULL)  return 0;

    capi_closesocket       = (wsfn_closesocket)GetProcAddress(windows_sockets_dll, "closesocket");

    if (capi_closesocket == NULL)  return 0;

    capi_connect           = (wsfn_connect)GetProcAddress(windows_sockets_dll, "connect");;

    if (capi_connect == NULL)  return 0;

    capi_ioctlsocket       = (wsfn_ioctlsocket)GetProcAddress(windows_sockets_dll, "ioctlsocket");

    if (capi_ioctlsocket == NULL)  return 0;

    capi_getsockopt        = (wsfn_getsockopt)GetProcAddress(windows_sockets_dll, "getsockopt");

    if (capi_getsockopt == NULL)  return 0;

    capi_htonl             = (wsfn_htonl)GetProcAddress(windows_sockets_dll, "htonl");

    if (capi_htonl == NULL)  return 0;

    capi_htons             = (wsfn_htons)GetProcAddress(windows_sockets_dll, "htons");

    if (capi_htons == NULL)  return 0;

    capi_inet_addr         = (wsfn_inet_addr)GetProcAddress(windows_sockets_dll, "inet_addr");

    if (capi_inet_addr == NULL)  return 0;

    capi_inet_ntoa         = (wsfn_inet_ntoa)GetProcAddress(windows_sockets_dll, "inet_ntoa");

    if (capi_inet_ntoa == NULL)  return 0;

    capi_listen            = (wsfn_listen)GetProcAddress(windows_sockets_dll, "listen");

    if (capi_listen == NULL)  return 0;

    capi_ntohl             = (wsfn_ntohl)GetProcAddress(windows_sockets_dll, "ntohl");

    if (capi_ntohl == NULL)  return 0;

    capi_ntohs             = (wsfn_ntohs)GetProcAddress(windows_sockets_dll, "ntohs");

    if (capi_ntohs == NULL)  return 0;

    capi_recv              = (wsfn_recv)GetProcAddress(windows_sockets_dll, "recv");

    if (capi_recv == NULL)  return 0;

    capi_recvfrom          = (wsfn_recvfrom)GetProcAddress(windows_sockets_dll, "recvfrom");

    if (capi_recvfrom == NULL)  return 0;

    capi_select            = (wsfn_select)GetProcAddress(windows_sockets_dll, "select");

    if (capi_select == NULL)  return 0;

    capi_send              = (wsfn_send)GetProcAddress(windows_sockets_dll, "send");

    if (capi_send == NULL)  return 0;

    capi_sendto            = (wsfn_sendto)GetProcAddress(windows_sockets_dll, "sendto");

    if (capi_sendto == NULL)  return 0;

    capi_setsockopt        = (wsfn_setsockopt)GetProcAddress(windows_sockets_dll, "setsockopt");

    if (capi_setsockopt == NULL)  return 0;

    capi_shutdown          = (wsfn_shutdown)GetProcAddress(windows_sockets_dll, "shutdown");

    if (capi_shutdown == NULL)  return 0;

    capi_socket            = (wsfn_socket)GetProcAddress(windows_sockets_dll, "socket");

    if (capi_socket == NULL)  return 0;

    capi_gethostbyaddr     = (wsfn_gethostbyaddr)GetProcAddress(windows_sockets_dll, "gethostbyaddr");

    if (capi_gethostbyaddr == NULL)  return 0;

    capi_gethostbyname     = (wsfn_gethostbyname)GetProcAddress(windows_sockets_dll, "gethostbyname");

    if (capi_gethostbyname == NULL)  return 0;

    capi_gethostname       = (wsfn_gethostname)GetProcAddress(windows_sockets_dll, "gethostname");

    if (capi_gethostname == NULL)  return 0;

    capi_getsockname       = (wsfn_getsockname)GetProcAddress(windows_sockets_dll, "getsockname");

    if (capi_getsockname == NULL)  return 0;


    capi_WSAStartup        = (wsfn_WSAStartup)GetProcAddress(windows_sockets_dll, "WSAStartup");

    if (capi_WSAStartup == NULL)  return 0;

    capi_WSACleanup        = (wsfn_WSACleanup)GetProcAddress(windows_sockets_dll, "WSACleanup");

    if (capi_WSACleanup == NULL)  return 0;

    capi_WSASetLastError   = (wsfn_WSASetLastError)GetProcAddress(windows_sockets_dll, "WSASetLastError");

    if (capi_WSASetLastError == NULL)  return 0;

    capi_WSAGetLastError   = (wsfn_WSAGetLastError)GetProcAddress(windows_sockets_dll, "WSAGetLastError");

    if (capi_WSAGetLastError == NULL)  return 0;



#else
    capi_accept            = (wsfn_accept)accept;
    capi_bind              = (wsfn_bind)bind;

    capi_closesocket       = (wsfn_closesocket)closesocket;

    capi_connect           = (wsfn_connect)connect;;

    capi_ioctlsocket       = (wsfn_ioctlsocket)ioctlsocket;

    capi_getsockopt        = (wsfn_getsockopt)getsockopt;

    capi_htonl             = (wsfn_htonl)htonl;

    capi_htons             = (wsfn_htons)htons;

    capi_inet_addr         = (wsfn_inet_addr)inet_addr;

    capi_inet_ntoa         = (wsfn_inet_ntoa)inet_ntoa;

    capi_listen            = (wsfn_listen)listen;

    capi_ntohl             = (wsfn_ntohl)ntohl;

    capi_ntohs             = (wsfn_ntohs)ntohs;

    capi_recv              = (wsfn_recv)recv;

    capi_recvfrom          = (wsfn_recvfrom)recvfrom;

    capi_select            = (wsfn_select)select;

    capi_send              = (wsfn_send)send;

    capi_sendto            = (wsfn_sendto)sendto;

    capi_setsockopt        = (wsfn_setsockopt)setsockopt;

    capi_shutdown          = (wsfn_shutdown)shutdown;

    capi_socket            = (wsfn_socket)socket;

    capi_gethostbyaddr     = (wsfn_gethostbyaddr)gethostbyaddr;

    capi_gethostbyname     = (wsfn_gethostbyname)gethostbyname;

    capi_gethostname       = (wsfn_gethostname)gethostname;

    capi_getsockname       = (wsfn_getsockname)getsockname;

    capi_WSAStartup        = (wsfn_WSAStartup)WSAStartup;

    capi_WSACleanup        = (wsfn_WSACleanup)WSACleanup;

    capi_WSASetLastError   = (wsfn_WSASetLastError)WSASetLastError;

    capi_WSAGetLastError   = (wsfn_WSAGetLastError)WSAGetLastError;

#endif

    return 1;

}
// END OF FUNCTION DEFINITIONS