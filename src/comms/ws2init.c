// Calls WSAStartup, makes sure we have a good version of WinSock2 DLL

// SYSTEM INCLUDES
//#include <WinSock.h>
//#include <WinBase.h>
//#include <StdIO.h>
#include <stdlib.h> // for EXIT_FAILURE
// END OF SYSTEM INCLUDES


// SIM INCLUDES
#include "CapiOpt.h"
//#include "capi.h"
#include "CapiPriv.h"
#include "WsProtos.h"
// END OF SIM INCLUDES



// GLOBAL VARIABLES
int windows_sockets_connections = 0;
extern com_API_last_error;
HINSTANCE windows_sockets_DLL = 0;
// END OF GLOBAL VARIABLES



// FUNCTION DECLARATIONS
static int CAPI_get_process_address(HINSTANCE windows_sockets_DLL);

wsfn_accept CAPI_accept = NULL;
wsfn_bind CAPI_bind = NULL;
wsfn_closesocket CAPI_closesocket = NULL;
wsfn_connect CAPI_connect = NULL;
wsfn_ioctlsocket CAPI_ioctlsocket = NULL;
wsfn_getsockopt CAPI_getsockopt = NULL;
wsfn_htonl CAPI_htonl = NULL;
wsfn_htons CAPI_htons = NULL;
wsfn_inet_addr CAPI_inet_addr = NULL;
wsfn_inet_ntoa CAPI_inet_ntoa = NULL;
wsfn_listen CAPI_listen = NULL;
wsfn_ntohl CAPI_ntohl = NULL;
wsfn_ntohs CAPI_ntohs = NULL;
wsfn_recv CAPI_recv = NULL;
wsfn_recvfrom CAPI_recvfrom = NULL;
wsfn_select CAPI_select = NULL;
wsfn_send CAPI_send = NULL;
wsfn_sendto CAPI_sendto = NULL;
wsfn_setsockopt CAPI_setsockopt = NULL;
wsfn_shutdown CAPI_shutdown = NULL;
wsfn_socket CAPI_socket = NULL;
wsfn_gethostbyaddr CAPI_gethostbyaddr = NULL;
wsfn_gethostbyname CAPI_gethostbyname = NULL;
wsfn_gethostname CAPI_gethostname = NULL;
wsfn_getsockname CAPI_getsockname = NULL;
wsfn_WSAStartup CAPI_WSAStartup = NULL;
wsfn_WSACleanup CAPI_WSACleanup = NULL;
wsfn_WSASetLastError CAPI_WSASetLastError = NULL;
wsfn_WSAGetLastError CAPI_WSAGetLastError = NULL;
// END OF FUNCTION DECLARATIONS



// FUNCTION DEFINITIONS
int initialize_windows_sockets(WSADATA* windows_socket_data)
{
	const char* DLL_NAME = "WSOCK32.DLL";
	const int MAJOR_VERSION = 1;
	const int MINOR_VERSION = 1;

	int wsaStatus;
    HINSTANCE  windows_sockets_dll = 0;


    if (!windows_sockets_connections) // No successful connection yet? 
    {
		TCHAR buffer_length[MAX_PATH] = "";
		DWORD error_code = EXIT_FAILURE;
		error_code = SearchPath(NULL, DLL_NAME, NULL, MAX_PATH,
								buffer_length, NULL);
		
		if (!error_code)
        {
            com_API_last_error = COMAPI_WINSOCKDLL_ERROR;
            return 0;
        }

#ifdef LOAD_DLLS
        windows_sockets_dll = LoadLibrary(DLL_NAME);

        if (windows_sockets_dll == NULL)
        {
            com_API_last_error = COMAPI_WINSOCKDLL_ERROR;
            return 0;
        }

#endif

        if (!CAPI_get_process_address(windows_sockets_dll))
        {
#ifdef LOAD_DLLS
            FreeLibrary(windows_sockets_dll);
            com_API_last_error = COMAPI_WINSOCKDLL_ERROR;
#endif
            return 0;
        }




        if (wsaStatus = CAPI_WSAStartup(MAKEWORD(MAJOR_VERSION, MINOR_VERSION), windows_socket_data))
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
                CAPI_WSACleanup();
                return 0;
            }

        }
    }

    /* if we get here , either we just need to increment counter
       or we execute the first successful WSAStartup */

    windows_sockets_connections++;


    return 1;

}


static int CAPI_get_process_address(HINSTANCE windows_sockets_dll)
{

#ifdef LOAD_DLLS

    CAPI_accept            = (wsfn_accept) GetProcAddress(windows_sockets_dll, "accept");

    if (CAPI_accept == NULL) return 0;

    CAPI_bind              = (wsfn_bind)GetProcAddress(windows_sockets_dll, "bind");

    if (CAPI_bind == NULL)  return 0;

    CAPI_closesocket       = (wsfn_closesocket)GetProcAddress(windows_sockets_dll, "closesocket");

    if (CAPI_closesocket == NULL)  return 0;

    CAPI_connect           = (wsfn_connect)GetProcAddress(windows_sockets_dll, "connect");;

    if (CAPI_connect == NULL)  return 0;

    CAPI_ioctlsocket       = (wsfn_ioctlsocket)GetProcAddress(windows_sockets_dll, "ioctlsocket");

    if (CAPI_ioctlsocket == NULL)  return 0;

    CAPI_getsockopt        = (wsfn_getsockopt)GetProcAddress(windows_sockets_dll, "getsockopt");

    if (CAPI_getsockopt == NULL)  return 0;

    CAPI_htonl             = (wsfn_htonl)GetProcAddress(windows_sockets_dll, "htonl");

    if (CAPI_htonl == NULL)  return 0;

    CAPI_htons             = (wsfn_htons)GetProcAddress(windows_sockets_dll, "htons");

    if (CAPI_htons == NULL)  return 0;

    CAPI_inet_addr         = (wsfn_inet_addr)GetProcAddress(windows_sockets_dll, "inet_addr");

    if (CAPI_inet_addr == NULL)  return 0;

    CAPI_inet_ntoa         = (wsfn_inet_ntoa)GetProcAddress(windows_sockets_dll, "inet_ntoa");

    if (CAPI_inet_ntoa == NULL)  return 0;

    CAPI_listen            = (wsfn_listen)GetProcAddress(windows_sockets_dll, "listen");

    if (CAPI_listen == NULL)  return 0;

    CAPI_ntohl             = (wsfn_ntohl)GetProcAddress(windows_sockets_dll, "ntohl");

    if (CAPI_ntohl == NULL)  return 0;

    CAPI_ntohs             = (wsfn_ntohs)GetProcAddress(windows_sockets_dll, "ntohs");

    if (CAPI_ntohs == NULL)  return 0;

    CAPI_recv              = (wsfn_recv)GetProcAddress(windows_sockets_dll, "recv");

    if (CAPI_recv == NULL)  return 0;

    CAPI_recvfrom          = (wsfn_recvfrom)GetProcAddress(windows_sockets_dll, "recvfrom");

    if (CAPI_recvfrom == NULL)  return 0;

    CAPI_select            = (wsfn_select)GetProcAddress(windows_sockets_dll, "select");

    if (CAPI_select == NULL)  return 0;

    CAPI_send              = (wsfn_send)GetProcAddress(windows_sockets_dll, "send");

    if (CAPI_send == NULL)  return 0;

    CAPI_sendto            = (wsfn_sendto)GetProcAddress(windows_sockets_dll, "sendto");

    if (CAPI_sendto == NULL)  return 0;

    CAPI_setsockopt        = (wsfn_setsockopt)GetProcAddress(windows_sockets_dll, "setsockopt");

    if (CAPI_setsockopt == NULL)  return 0;

    CAPI_shutdown          = (wsfn_shutdown)GetProcAddress(windows_sockets_dll, "shutdown");

    if (CAPI_shutdown == NULL)  return 0;

    CAPI_socket            = (wsfn_socket)GetProcAddress(windows_sockets_dll, "socket");

    if (CAPI_socket == NULL)  return 0;

    CAPI_gethostbyaddr     = (wsfn_gethostbyaddr)GetProcAddress(windows_sockets_dll, "gethostbyaddr");

    if (CAPI_gethostbyaddr == NULL)  return 0;

    CAPI_gethostbyname     = (wsfn_gethostbyname)GetProcAddress(windows_sockets_dll, "gethostbyname");

    if (CAPI_gethostbyname == NULL)  return 0;

    CAPI_gethostname       = (wsfn_gethostname)GetProcAddress(windows_sockets_dll, "gethostname");

    if (CAPI_gethostname == NULL)  return 0;

    CAPI_getsockname       = (wsfn_getsockname)GetProcAddress(windows_sockets_dll, "getsockname");

    if (CAPI_getsockname == NULL)  return 0;


    CAPI_WSAStartup        = (wsfn_WSAStartup)GetProcAddress(windows_sockets_dll, "WSAStartup");

    if (CAPI_WSAStartup == NULL)  return 0;

    CAPI_WSACleanup        = (wsfn_WSACleanup)GetProcAddress(windows_sockets_dll, "WSACleanup");

    if (CAPI_WSACleanup == NULL)  return 0;

    CAPI_WSASetLastError   = (wsfn_WSASetLastError)GetProcAddress(windows_sockets_dll, "WSASetLastError");

    if (CAPI_WSASetLastError == NULL)  return 0;

    CAPI_WSAGetLastError   = (wsfn_WSAGetLastError)GetProcAddress(windows_sockets_dll, "WSAGetLastError");

    if (CAPI_WSAGetLastError == NULL)  return 0;



#else
    CAPI_accept            = (wsfn_accept)accept;
    CAPI_bind              = (wsfn_bind)bind;

    CAPI_closesocket       = (wsfn_closesocket)closesocket;

    CAPI_connect           = (wsfn_connect)connect;;

    CAPI_ioctlsocket       = (wsfn_ioctlsocket)ioctlsocket;

    CAPI_getsockopt        = (wsfn_getsockopt)getsockopt;

    CAPI_htonl             = (wsfn_htonl)htonl;

    CAPI_htons             = (wsfn_htons)htons;

    CAPI_inet_addr         = (wsfn_inet_addr)inet_addr;

    CAPI_inet_ntoa         = (wsfn_inet_ntoa)inet_ntoa;

    CAPI_listen            = (wsfn_listen)listen;

    CAPI_ntohl             = (wsfn_ntohl)ntohl;

    CAPI_ntohs             = (wsfn_ntohs)ntohs;

    CAPI_recv              = (wsfn_recv)recv;

    CAPI_recvfrom          = (wsfn_recvfrom)recvfrom;

    CAPI_select            = (wsfn_select)select;

    CAPI_send              = (wsfn_send)send;

    CAPI_sendto            = (wsfn_sendto)sendto;

    CAPI_setsockopt        = (wsfn_setsockopt)setsockopt;

    CAPI_shutdown          = (wsfn_shutdown)shutdown;

    CAPI_socket            = (wsfn_socket)socket;

    CAPI_gethostbyaddr     = (wsfn_gethostbyaddr)gethostbyaddr;

    CAPI_gethostbyname     = (wsfn_gethostbyname)gethostbyname;

    CAPI_gethostname       = (wsfn_gethostname)gethostname;

    CAPI_getsockname       = (wsfn_getsockname)getsockname;

    CAPI_WSAStartup        = (wsfn_WSAStartup)WSAStartup;

    CAPI_WSACleanup        = (wsfn_WSACleanup)WSACleanup;

    CAPI_WSASetLastError   = (wsfn_WSASetLastError)WSASetLastError;

    CAPI_WSAGetLastError   = (wsfn_WSAGetLastError)WSAGetLastError;

#endif

    return 1;

}
// END OF FUNCTION DEFINITIONS