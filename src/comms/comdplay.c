//#define INITGUID

#pragma optimize( "", off ) // JB 010718

#pragma warning(disable : 4115)
#pragma warning(disable : 4706)
#pragma warning(disable : 4189)

#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>		
#include <conio.h>
#include <time.h>

#include "capiopt.h"
#include "capi.h"
#include "capipriv.h"
//sfr: added bw
#include "capibwcontrol.h"

#include <objbase.h>
#include <cguid.h>
#pragma warning(disable : 4201)
#include "dplay.h"
#include "dplobby.h"

#include "comdplay.h"

#define TIMESTAMP
extern int ComAPILastError;
#ifdef TIMESTAMP
extern DWProc_t CAPI_TimeStamp;
#endif
static HINSTANCE hDPLAYDLL=0;
static HINSTANCE hOLE32DLL=0;

#define NAMEMAX			200			// string size
#define SEND 0
#define RECV 1

#define ENUMERATE_PROTOCOLS  CAPI_LAST_PROTOCOL+1
#define MAXENUMTIMEOUTS 4
#define MAXOPENTIMEOUTS 4
#define COMMS           1

static COMMS_COINITIALIZED = 0;

extern void enter_cs (void);
extern void leave_cs (void);

typedef struct {
	LPDIRECTPLAY3A		lpDPlay;
	GUID				guidInstance;
	int                 timeouts;
} STATUSCONTEXT, *LPSTATUSCONTEXT;

static int doing_host_or_join = FALSE;

// guid for this application
// {126E6180-D307-11d0-9C4F-00A0C905425E}
DEFINE_GUID(OVERRIDE_GUID, 
0x126e6180, 0xd307, 0x11d0, 0x9c, 0x4f, 0x0, 0xa0, 0xc9, 0x5, 0x42, 0x5e);

// guid for this application
// {5BFDB060-06A4-11d0-9C4F-00A0C905425E}
DEFINE_GUID(DPCHAT_GUID, 
0x5bfdb060, 0x6a4, 0x11d0, 0x9c, 0x4f, 0x0, 0xa0, 0xc9, 0x5, 0x42, 0x5e);

// prototypes





void				ComDPLAYClose(ComAPIHandle c);
int					ComDPLAYSend(ComAPIHandle c, int msgsize, int oob);
int					ComDPLAYSendX(ComAPIHandle c, int msgsize,ComAPIHandle Xcom);
char				*ComDPLAYSendBufferGet(ComAPIHandle c);
char				*ComDPLAYRecvBufferGet(ComAPIHandle c);
int					ComDPLAYRecv(ComAPIHandle c);
unsigned long		ComDPLAYQuery(ComAPIHandle c, int querytype);
int					ComDPLAYHostIDGet(ComAPIHandle c, char *buf, int reset);
char				*ComErrorString(HRESULT hr);
static void			ComSetSerialData(LPDPCOMPORTADDRESS ComPortAddress,char *datastring);


HRESULT				InitializeOverride(ComDPLAY *c);

HRESULT				DoHostOrJoin (ComDPLAY *c);

BOOL FAR PASCAL		DirectPlayEnumerateCallback(LPGUID lpSPGuid,
                        LPTSTR lpszSPName,
						DWORD dwMajorVersion,
                        DWORD dwMinorVersion, LPVOID lpContext);

HRESULT				GetModemNames( LPDIRECTPLAYLOBBY2A lpDPlayLobby,ComDPLAY *c);

BOOL FAR PASCAL		EnumSessionsCallback(LPCDPSESSIONDESC2	lpSessionDesc,
						LPDWORD				lpdwTimeOut,
						DWORD				dwFlags,
						LPVOID				lpContext);


int					HandleApplicationMessage(ComDPLAY *lpDPInfo, LPDPMSG_GENERIC lpMsg,
						DWORD dwMsgSize, DPID idFrom, DPID idTo);

int					HandleSystemMessage(ComDPLAY *lpDPInfo, LPDPMSG_GENERIC lpMsg,
						DWORD dwMsgSize, DPID idFrom, DPID idTo);

int					ReceiveMessage(ComDPLAY *lpDPInfo);

DWORD WINAPI		ReceiveThread(LPVOID lpThreadParameter);

int					d3dDbgLevel=0;

HANDLE				ghKillReceiveEvent=0;


#define DEBUGSTDOUTALSO       // define to also send debug to stdout a
                                // when DEBUGTODEBUGGER is on

static   LPDIRECTPLAY3A			lpDPlayCOMM = NULL;            /* IDirectPlay3A interface pointer */
static   LPDIRECTPLAYLOBBY2A	lpDPlayLobbyCOMM = NULL;     
 
static void			UnLoadDLLs(void)
{

#ifdef LOAD_DLLS
   if(hDPLAYDLL) FreeLibrary(hDPLAYDLL);
   if(hOLE32DLL) FreeLibrary(hOLE32DLL);
#endif
    hDPLAYDLL = 0;
    hOLE32DLL = 0;
}



#ifdef TIMESTAMP
unsigned long ComDPLAYGetTimeStamp(ComAPIHandle c)
{
  if(c)
  {
	  ComDPLAY *lpDPInfo = (ComDPLAY *)c;

	  return lpDPInfo->timestamp;
  }
  else
  {
	  return 0;
  }
}
#endif



// VP_changes this is important Load DLL
static int LoadDLLs(void)
{
#ifdef LOAD_DLLS

       int buflen;

       if(hDPLAYDLL && hOLE32DLL) return 1;
       buflen = SearchPath(NULL, "DPLAYX.DLL", NULL,0,NULL,NULL);	

       if(buflen == 0 )
       {
          ComAPILastError = COMAPI_DPLAYDLL_ERROR;
          return 0;
       }
      
       hDPLAYDLL = LoadLibrary("DPLAYX.DLL");
       if (hDPLAYDLL == NULL) {
      
          ComAPILastError = COMAPI_DPLAYDLL_ERROR;
          return 0;
       }
       
       CAPI_DirectPlayLobbyCreate            = (DPFN_DirectPlayLobbyCreate) GetProcAddress(hDPLAYDLL,"DirectPlayLobbyCreateA");
       if (CAPI_DirectPlayLobbyCreate == NULL )
       {
           ComAPILastError = COMAPI_DPLAYDLL_ERROR;
           return 0;
       }
       CAPI_DirectPlayEnumerate            = (DPFN_DirectPlayEnumerate) GetProcAddress(hDPLAYDLL,"DirectPlayEnumerateA");
       if (CAPI_DirectPlayEnumerate == NULL )
       { 
          ComAPILastError = COMAPI_DPLAYDLL_ERROR;
          return 0;
       }
       CAPI_DirectPlayCreate            = (DPFN_DirectPlayCreate) GetProcAddress(hDPLAYDLL,"DirectPlayCreate");
       if (CAPI_DirectPlayCreate == NULL )
       {
           ComAPILastError = COMAPI_DPLAYDLL_ERROR;
           return 0;
       }


       buflen = SearchPath(NULL, "OLE32.DLL", NULL,0,NULL,NULL);	
       if(buflen == 0 )
       {
          ComAPILastError = COMAPI_OLE32DLL_ERROR;
          return 0;
       }

       hOLE32DLL = LoadLibrary("OLE32.DLL");
       if (hOLE32DLL == NULL) {
      
          ComAPILastError = COMAPI_OLE32DLL_ERROR;
          return 0;
       }
       
       CAPI_CoInitialize            =  (OLEFN_CoInitialize)GetProcAddress(hOLE32DLL,"CoInitialize");
       if (CAPI_CoInitialize == NULL )
       {
           ComAPILastError = COMAPI_OLE32DLL_ERROR;
           return 0;
       }

       CAPI_CoUninitialize            = (OLEFN_CoUninitialize)GetProcAddress(hOLE32DLL,"CoUninitialize");
       if (CAPI_CoUninitialize == NULL )
       {
           ComAPILastError = COMAPI_OLE32DLL_ERROR;
           return 0;
       }
       CAPI_CoCreateInstance            = (OLEFN_CoCreateInstance) GetProcAddress(hOLE32DLL,"CoCreateInstance");
       if (CAPI_CoCreateInstance == NULL )
       {
           ComAPILastError = COMAPI_OLE32DLL_ERROR;
           return 0;
       }

       return 1; /* SUCCESS */

#else
        CAPI_DirectPlayLobbyCreate = DirectPlayLobbyCreate;
        CAPI_DirectPlayEnumerate   =  DirectPlayEnumerate;
        CAPI_DirectPlayCreate      =  DirectPlayCreate;
        CAPI_CoInitialize          =  CoInitialize;
        CAPI_CoUninitialize        =  CoUninitialize;
        CAPI_CoCreateInstance      =  CoCreateInstance;
        return 1;
#endif
}


ComAPIHandle ComDPLAYOpenHost(int protocol,char *address, int buffersize, void *guid, void (*HostCallback)(ComAPIHandle c, int ret), int timeoutsecs)
{
		return ComDPLAYOpen(protocol,CAPI_HOST, address,buffersize, guid, HostCallback,timeoutsecs);
}
                                                                            


ComAPIHandle ComDPLAYOpen(int protocol,int mode, char *address, int buffersize, void *guid, void (*ConnectCallback)(ComAPIHandle c, int ret), int timeoutsecs)
{
//		int			iResult = 0;
		ComDPLAY    *c;
    
		if(!LoadDLLs()) return NULL;

		if (doing_host_or_join) // JB 010718
		{
			return NULL;
		}

		enter_cs ();

		/* allocate a new Comhandle struct */
		c = (ComDPLAY *)malloc(sizeof(ComDPLAY));
		memset(c,0,sizeof(ComDPLAY));
		if (protocol == CAPI_DPLAY_MODEM_PROTOCOL )
		{
		if(strlen(address) < MAXADDRESS)	strcpy(c->phonenumber,address);
		}
		else if (protocol == CAPI_DPLAY_TCP_PROTOCOL)
		{
		if(strlen(address) < MAXADDRESS)	strcpy(c->IPAddress,address);
		}
		else if (protocol == CAPI_DPLAY_SERIAL_PROTOCOL)
		{
		if(strlen(address) < MAXSERIALDATA)  strcpy(c->SerialData,address);
		}


		c->lpDPlay							= NULL;
		c->lpDPlayLobby						= NULL;
		c->buffer_size						= sizeof(modemHeader) + buffersize;
		c->bIsHost							= mode;
		memcpy(&(c->guidApplication),guid,sizeof(GUID));


		/* initialize header data */
		c->apiheader.protocol				= protocol;
		c->apiheader.send_func				= ComDPLAYSend;
		c->apiheader.sendX_func				= ComDPLAYSendX;
		c->apiheader.recv_func				= ComDPLAYRecv;
		c->apiheader.send_buf_func			= ComDPLAYSendBufferGet;
		c->apiheader.recv_buf_func			= ComDPLAYRecvBufferGet;
		c->apiheader.addr_func				= ComDPLAYHostIDGet;
		c->apiheader.close_func				= ComDPLAYClose;
		c->apiheader.query_func				= ComDPLAYQuery;
#ifdef TIMESTAMP
		c->apiheader.get_timestamp_func     = ComDPLAYGetTimeStamp;
#endif
		c->connect_callback_func			= ConnectCallback;                        

		c->recv_buffer						= (char*)malloc(buffersize + 1);
		c->send_buffer						= (char*)malloc(buffersize + 1);
		c->state							= COMAPI_STATE_CONNECTION_PENDING;
		c->close_status 					= 0;
		c->closer		 					= 0;
        c->timeoutsecs                      = (short) timeoutsecs;

		doing_host_or_join = TRUE;
			
		c->ghConnectThread					= CreateThread(NULL,0,

		(LPTHREAD_START_ROUTINE)DoHostOrJoin,(LPVOID)c,0,&c->gidConnectThread);

		if (c->ghConnectThread == NULL)
		{
//		    DWORD error = GetLastError();	   
		}

		leave_cs ();

		return (ComAPIHandle)c;
}



void ComDPLAYClose(ComAPIHandle c)
{
		ComDPLAY *lpDPInfo = (ComDPLAY *)c;


		if (lpDPInfo->close_status == LDPLAY_WAIT_FOR_PLAYER)  /* no player joined yet */
		{
	      lpDPInfo->dpidPlayerRemote = 1;
		  while (lpDPInfo->close_status != LDPLAY_OK_TO_CLOSE);
		}
		lpDPInfo->close_status = 0;

		if (lpDPInfo->ghReceiveThread)
		{
			// wake up receive thread and wait for it to quit
			SetEvent(lpDPInfo->ghKillReceiveEvent);
			WaitForSingleObject(lpDPInfo->ghReceiveThread, INFINITE);

			CloseHandle(lpDPInfo->ghReceiveThread);
			lpDPInfo->ghReceiveThread = NULL;
		}



		if (lpDPInfo->ghKillReceiveEvent)
		{
			CloseHandle(lpDPInfo->ghKillReceiveEvent);
			lpDPInfo->ghKillReceiveEvent = NULL;
		}


		if (lpDPInfo->lpDPlay)
		{
			if (lpDPInfo->dpidPlayer && lpDPInfo->status == LDPLAY_PLAYER_CREATED)
			{
				lpDPInfo->lpDPlay->lpVtbl->DestroyPlayer(lpDPInfo->lpDPlay,lpDPInfo->dpidPlayer);
				lpDPInfo->dpidPlayer = 0;
			}


			if (lpDPInfo->status >= LDPLAY_OPENED)
			{
				lpDPInfo->lpDPlay->lpVtbl->Close(lpDPInfo->lpDPlay);
			}
				lpDPInfo->lpDPlay->lpVtbl->Close(lpDPInfo->lpDPlay);
				lpDPInfo->lpDPlay->lpVtbl->Release(lpDPInfo->lpDPlay);
				lpDPInfo->lpDPlay = NULL;
                lpDPlayCOMM = NULL;
		}

		lpDPInfo->status = 0;

		if (lpDPInfo->hPlayerEvent)
		{
			CloseHandle(lpDPInfo->hPlayerEvent);
			lpDPInfo->hPlayerEvent = NULL;
		}

  
        if(lpDPInfo->modemName)
            free(lpDPInfo->modemName);

        if(lpDPInfo->recv_buffer)
           free(lpDPInfo->recv_buffer);

      //  if(lpDPInfo->send_buffer); /* interesting - errant semi-colon causes free() to assert */
          if(lpDPInfo->send_buffer)
           free(lpDPInfo->send_buffer);

		free(lpDPInfo);

			if(COMMS_COINITIALIZED)
			{
				CAPI_CoUninitialize();
				COMMS_COINITIALIZED = 0;
			}


		UnLoadDLLs();
		return;
}





HRESULT InitializeOverride(ComDPLAY *c)
{
		LPDIRECTPLAYLOBBYA	lpDPlayLobbyA = NULL;
		LPDIRECTPLAYLOBBY2A	lpDPlayLobby2A = NULL;
		HRESULT				hr;
			
		// get ANSI DirectPlayLobby interface
		hr = CAPI_DirectPlayLobbyCreate(NULL, &lpDPlayLobbyA, NULL, NULL, 0);
		if FAILED(hr)
			goto FAILURE;

		// get ANSI DirectPlayLobby2 interface
		hr = lpDPlayLobbyA->lpVtbl->QueryInterface(lpDPlayLobbyA,
								&IID_IDirectPlayLobby2A, (LPVOID *) &lpDPlayLobby2A);
		if FAILED(hr)
			goto FAILURE;

		// don't need DirectPlayLobby interface anymore
		lpDPlayLobbyA->lpVtbl->Release(lpDPlayLobbyA);
		lpDPlayLobbyA = NULL;

		// put all the service providers in combo box
		CAPI_DirectPlayEnumerate(DirectPlayEnumerateCallback, c);

		// fill modem combo box with available modems
		GetModemNames(lpDPlayLobby2A,c);


		// return the ANSI lobby interface
		c->lpDPlayLobby = lpDPlayLobby2A;

		return DP_OK;

FAILURE:
		if (lpDPlayLobbyA)
			lpDPlayLobbyA->lpVtbl->Release(lpDPlayLobbyA);
		if (lpDPlayLobby2A)
			lpDPlayLobby2A->lpVtbl->Release(lpDPlayLobby2A);

		return hr;
}


// ---------------------------------------------------------------------------
// DirectPlayEnumerateCallback
// ---------------------------------------------------------------------------
// Description:             Enumeration callback called by DirectPlay.
//							Enumerates service providers registered with DirectPlay.
// Arguments:
//  LPGUID					[in] GUID of service provider
//  LPTSTR					[in] name of service provider
//  DWORD					[in] major version of DirectPlay
//  DWORD					[in] minor version of DirectPlay
//  LPVOID				    [in] user-defined context
// Returns:
//  BOOL					TRUE to continue enumerating
BOOL FAR PASCAL DirectPlayEnumerateCallback(
						LPGUID		lpSPGuid,
						LPTSTR		lpszSPName,
						DWORD		   dwMajorVersion,
						DWORD		   dwMinorVersion,
						LPVOID		lpContext)
{
		ComDPLAY 			*c = (ComDPLAY *) lpContext;

		dwMajorVersion;
		dwMinorVersion;	// eliminate warnings;

		if (c->apiheader.protocol == ENUMERATE_PROTOCOLS)
		{
			 ComDPLAYENUM *cenum;
			 int proto;

			 proto = 0;
			 cenum =   (ComDPLAYENUM *)lpContext;
			 if(strstr(lpszSPName,"Modem") || strstr(lpszSPName,"modem") ||  strstr(lpszSPName,"MODEM"))
				   proto = CAPI_DPLAY_MODEM_PROTOCOL;
			 if(strstr(lpszSPName,"TCP") || strstr(lpszSPName,"Tcp") ||  strstr(lpszSPName,"tcp"))
				   proto = CAPI_DPLAY_TCP_PROTOCOL;
			 if(strstr(lpszSPName,"Serial") || strstr(lpszSPName,"serial") ||  strstr(lpszSPName,"SERIAL"))
				   proto = CAPI_DPLAY_SERIAL_PROTOCOL;


			 if (proto && (cenum->count < cenum->maxprotocols) )
			 {
				*(cenum->protocols) = proto;
				cenum->count++;
				cenum->protocols++;
			 }



		}
		// make space for application GUID
		else if (c->apiheader.protocol == CAPI_DPLAY_MODEM_PROTOCOL)
		{
		  if(strstr(lpszSPName,"modem") || strstr(lpszSPName,"Modem") 
				||  strstr(lpszSPName,"MODEM"))
		  {
				c->guidServiceProvider = *lpSPGuid;
		  }
		}

		else if (c->apiheader.protocol == CAPI_DPLAY_TCP_PROTOCOL)
		{

		  if(strstr(lpszSPName,"TCP") || strstr(lpszSPName,"tcp") )
		  {
				c->guidServiceProvider = *lpSPGuid;
		  }

		}

		else if (c->apiheader.protocol == CAPI_DPLAY_SERIAL_PROTOCOL)
		{
			if(strstr(lpszSPName,"Serial") || strstr(lpszSPName,"SERIAL") ||strstr(lpszSPName,"serial") )
			{
				 c->guidServiceProvider = *lpSPGuid;
			}

		}


		return TRUE;
}





BOOL FAR PASCAL EnumSessionsCallback(
						LPCDPSESSIONDESC2	lpSessionDesc,
						LPDWORD				lpdwTimeOut,
						DWORD				dwFlags,
						LPVOID				lpContext)
{
		HWND			hWnd = (HWND) lpContext;
		LPGUID			lpGuid;
		STATUSCONTEXT   *lpstatusContext;

		hWnd;  // eliminate warning

		lpstatusContext = (STATUSCONTEXT *)lpContext;

		// see if last session has been enumerated
		if (dwFlags & DPESC_TIMEDOUT)
		{
			if(lpstatusContext->timeouts < MAXENUMTIMEOUTS )
			{
				*lpdwTimeOut = 0;
				lpstatusContext->timeouts++;
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}

		// make space for session instance guid
		lpGuid = (LPGUID) malloc( sizeof(GUID) );
		if (lpGuid == NULL)
			goto FAILURE;

		// store pointer to guid in list
		lpstatusContext->guidInstance = lpSessionDesc->guidInstance;
        lpstatusContext->timeouts = MAXENUMTIMEOUTS; 
		FAILURE:
		return TRUE;
}


BOOL WINAPI EnumPlayersCallback2(
	DPID dpId,
	DWORD dwPlayerType,
	LPCDPNAME lpName,
	DWORD dwFlags,
	LPVOID lpContext
	)
{
		ComDPLAY *c =  (ComDPLAY *)lpContext;

		dwPlayerType;
		lpName;		   //eliminate warnings
		dwFlags;

		c->dpidPlayerRemote = dpId;
		return TRUE;

}



HRESULT	CreateServiceProviderAddress(LPDIRECTPLAYLOBBY2A lpDPlayLobby,
									 LPVOID *lplpAddress, LPDWORD
                                     lpdwAddressSize,void *comhandle)
{
		DPCOMPOUNDADDRESSELEMENT	addressElements[3];
		CHAR						szIPAddressString[NAMEMAX];
		CHAR						szPhoneNumberString[NAMEMAX];
		CHAR						szModemString[NAMEMAX];
		LPVOID						lpAddress = NULL;
		DWORD						dwAddressSize = 0;
		DWORD						dwElementCount;

		HRESULT						hr;
		DPCOMPORTADDRESS            ComPortAddress;

		ComDPLAY  *c = (ComDPLAY *) comhandle;

		dwElementCount = 0;

		if (IsEqualGUID(&c->guidServiceProvider, &DPSPGUID_MODEM))
		{
			// Modem needs a service provider, a phone number string and a modem string

			// service provider
			addressElements[dwElementCount].guidDataType = DPAID_ServiceProvider;
			addressElements[dwElementCount].dwDataSize = sizeof(GUID);
			addressElements[dwElementCount].lpData = (LPVOID) &DPSPGUID_MODEM;
			dwElementCount++;

			// add a modem string if available

			lstrcpy(szModemString, c->modemName);
			{
				addressElements[dwElementCount].guidDataType = DPAID_Modem;
				addressElements[dwElementCount].dwDataSize = lstrlen(szModemString) + 1;
				addressElements[dwElementCount].lpData = szModemString;
				dwElementCount++;
			}

			// add phone number string
			lstrcpy(szPhoneNumberString,c->phonenumber);
			addressElements[dwElementCount].guidDataType = DPAID_Phone;
			addressElements[dwElementCount].dwDataSize = lstrlen(szPhoneNumberString) + 1;
			addressElements[dwElementCount].lpData = szPhoneNumberString;
			dwElementCount++;
		}

		// internet TCP/IP service provider
		else if (IsEqualGUID(&c->guidServiceProvider, &DPSPGUID_TCPIP))
		{
			// TCP/IP needs a service provider and an IP address

			// service provider
			addressElements[dwElementCount].guidDataType = DPAID_ServiceProvider;
			addressElements[dwElementCount].dwDataSize = sizeof(GUID);
			addressElements[dwElementCount].lpData = (LPVOID) &DPSPGUID_TCPIP;
			dwElementCount++;

			// IP address string
			lstrcpy(szIPAddressString, c->IPAddress);
			addressElements[dwElementCount].guidDataType = DPAID_INet;
			addressElements[dwElementCount].dwDataSize = lstrlen(szIPAddressString) + 1;
			addressElements[dwElementCount].lpData = szIPAddressString;
			dwElementCount++;
		}

		/* SERIAL CONNECTION */
		else if (IsEqualGUID(&c->guidServiceProvider, &DPSPGUID_SERIAL))
		{

				// service provider
			addressElements[dwElementCount].guidDataType = DPAID_ServiceProvider;
			addressElements[dwElementCount].dwDataSize = sizeof(GUID);
			addressElements[dwElementCount].lpData = (LPVOID) &DPSPGUID_SERIAL;
			dwElementCount++;

			// ComPort Data
			ComSetSerialData(&ComPortAddress,c->SerialData);

			addressElements[dwElementCount].guidDataType = DPAID_ComPort;
			addressElements[dwElementCount].dwDataSize =sizeof( DPCOMPORTADDRESS);
			addressElements[dwElementCount].lpData = (LPVOID)&ComPortAddress ;
			dwElementCount++;
		}



		// IPX service provider
		else if (IsEqualGUID(&c->guidServiceProvider, &DPSPGUID_IPX))
		{
			// IPX just needs a service provider

			// service provider
			addressElements[dwElementCount].guidDataType = DPAID_ServiceProvider;
			addressElements[dwElementCount].dwDataSize = sizeof(GUID);
			addressElements[dwElementCount].lpData = (LPVOID) &DPSPGUID_IPX;
			dwElementCount++;
		}




		// anything else, let service provider collect settings, if any
		else
		{
			// service provider
			addressElements[dwElementCount].guidDataType = DPAID_ServiceProvider;
			addressElements[dwElementCount].dwDataSize = sizeof(GUID);
			addressElements[dwElementCount].lpData = (LPVOID) &c->guidServiceProvider;
			dwElementCount++;
		}

		// see how much room is needed to store this address
		hr = lpDPlayLobby->lpVtbl->CreateCompoundAddress(lpDPlayLobby,
							addressElements, dwElementCount,
							NULL, &dwAddressSize);
		if (hr != DPERR_BUFFERTOOSMALL)
			goto FAILURE;

		// allocate space
		lpAddress = malloc( dwAddressSize);
		if (lpAddress == NULL)
		{
			hr = DPERR_NOMEMORY;
			goto FAILURE;
		}

		// create the address
		hr = lpDPlayLobby->lpVtbl->CreateCompoundAddress(lpDPlayLobby,
							addressElements, dwElementCount,
							lpAddress, &dwAddressSize);
		if FAILED(hr)
			goto FAILURE;

		// return the address info
		*lplpAddress = lpAddress;
		*lpdwAddressSize = dwAddressSize;

		return DP_OK;

		FAILURE:
		if (lpAddress)
			free (lpAddress);

		return hr;
}

HRESULT	DoHostOrJoin(ComDPLAY *c)
{
	LPDIRECTPLAY3A	lpDPlay = NULL;
	LPVOID			lpAddress = NULL;
	MSG				msg;
	DWORD			dwAddressSize = 0;
	DPSESSIONDESC2	sessionDesc;
	STATUSCONTEXT	statusContext;
	HRESULT			hr;
	int				firstmessage = 1;
	//		int				ReturnCode = 0;
	int				AttemptOpen;
	int             bIsHost;
	
	
	DPCAPS    DPCaps;
	clock_t starttime, endtime, starttime2;
	
	bIsHost = c->bIsHost;
	/* initialize COM library */
	if(! COMMS_COINITIALIZED)
	{
		hr = CAPI_CoInitialize(NULL);
		if FAILED(hr)
			goto FAILURE;
		COMMS_COINITIALIZED = 1;
	}
	
	hr = InitializeOverride(c);
	
	if FAILED(hr)
	{
		goto FAILURE;
	}
	
	
	
	
	
	
	
	// bail if we don't have a lobby interface
	if (c->lpDPlayLobby == NULL)
	{
		hr = DPERR_INVALIDOBJECT;
		goto FAILURE;
	}
	
	/* Create Player event */
	c->hPlayerEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	if (c->hPlayerEvent == NULL)
	{
		goto FAILURE;
	}
	
	c->ghReceiveThread = NULL;
	
	
#ifdef THREADED
	/* Create Kill Thread event */
	c->ghKillReceiveEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	if (c->ghKillReceiveEvent == NULL)
	{
		goto FAILURE;
	}
	c->ghReceiveThread = CreateThread(NULL,0,ReceiveThread,c,0,&c->gidReceiveThread);
#endif
	
	
	// get service provider address from information in dialog
	hr = CreateServiceProviderAddress(c->lpDPlayLobby, &lpAddress,
		&dwAddressSize,(void *)c);
	if FAILED(hr)
		goto FAILURE;
	
		/**
		// interface already exists, so release it
		if (c->lpDPlay)
		{
		(c->lpDPlay)->lpVtbl->Release(c->lpDPlay);
		c->lpDPlay = NULL;
		}
	**/
	if (lpDPlayCOMM == NULL)
	{
		// create an ANSI DirectPlay3 interface
		hr = CAPI_CoCreateInstance(&CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, 
			&IID_IDirectPlay3A, (LPVOID*)&lpDPlay);
		if FAILED(hr)
			goto FAILURE;
		lpDPlayCOMM = lpDPlay;
	}
	else
		lpDPlay = lpDPlayCOMM;
	
	c->lpDPlay = lpDPlay;
	
	// initialize the connection using the address
	hr = lpDPlay->lpVtbl->InitializeConnection(lpDPlay, lpAddress, 0);
	if(!(hr == DPERR_ALREADYINITIALIZED || hr == DP_OK))
		goto FAILURE;
	
	if (c->bIsHost)
	{
		// host a new session
		ZeroMemory(&sessionDesc, sizeof(DPSESSIONDESC2));
		sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
		sessionDesc.dwFlags = DPSESSION_MIGRATEHOST | DPSESSION_KEEPALIVE;
		sessionDesc.guidApplication = c->guidApplication;
		//		sessionDesc.guidApplication = OVERRIDE_GUID;
		sessionDesc.dwMaxPlayers = 0;
		sessionDesc.lpszSessionNameA = "Override";
		
		// open it
		hr = lpDPlay->lpVtbl->Open(lpDPlay, &sessionDesc, DPOPEN_CREATE);
		if (hr == DP_OK) 
			c->status = LDPLAY_OPENED;
		
#ifdef THREADED
		hr = lpDPlay->lpVtbl->CreatePlayer(lpDPlay, &c->dpidPlayer,NULL,c->hPlayerEvent,NULL,0,0);
#else
		hr = lpDPlay->lpVtbl->CreatePlayer(lpDPlay, &c->dpidPlayer,NULL,NULL,NULL,0,0);
#endif
		
		if (hr == DP_OK)
			c->status = LDPLAY_PLAYER_CREATED;
		
		starttime = clock();
		c->close_status = LDPLAY_WAIT_FOR_PLAYER;
		while (c->dpidPlayerRemote == 0)
		{
			if (PeekMessage( &msg, NULL, 0, 0,PM_NOREMOVE ))
			{
				GetMessage( &msg, NULL, 0, 0 );
				TranslateMessage( &msg );
				DispatchMessage( &msg );
				firstmessage = 0;
			}
			else
			{
#ifndef THREADED
				ComDPLAYRecv((ComAPIHandle)c);
#endif
			}
			
			endtime = clock();
			if(endtime - starttime > 1000 * c->timeoutsecs)
			{
				hr = COMAPI_CONNECTION_TIMEOUT;
				c->close_status = LDPLAY_OK_TO_CLOSE;
				
				goto FAILURE;
			}
		} /* while */
		
		c->close_status = LDPLAY_OK_TO_CLOSE;
		
		
		
	}  /* isHost */
	
	// enumerate the sessions
	else    /* JOINer */
	{
		// enum sessions
		ZeroMemory(&sessionDesc, sizeof(DPSESSIONDESC2));
		sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
		//			sessionDesc.guidApplication = OVERRIDE_GUID;
		//			sessionDesc.guidApplication = c->guidApplication;
		sessionDesc.guidApplication = GUID_NULL;
		
		starttime2 = starttime = clock();
		statusContext.timeouts = 0;
		do
		{
			
			hr = lpDPlay->lpVtbl->EnumSessions(lpDPlay, &sessionDesc, 0,
				EnumSessionsCallback, &statusContext,
				DPENUMSESSIONS_AVAILABLE 
				//									| DPENUMSESSIONS_ASYNC 
				| DPENUMSESSIONS_RETURNSTATUS);
			if (hr == DPERR_CONNECTING) 
			{
				if (PeekMessage( &msg, NULL, 0, 0,PM_NOREMOVE ))
				{
					
					GetMessage(&msg, NULL, 0, 0);
					TranslateMessage( &msg );
					DispatchMessage( &msg );
				}
			}
			
			endtime = clock();
			if(endtime - starttime > 1000 * c->timeoutsecs)
			{
				hr = COMAPI_CONNECTION_TIMEOUT;
				
				goto FAILURE;
			}
			
			
		} while( hr == DPERR_CONNECTING );
		
		if FAILED(hr)
			goto FAILURE;
		
		// open the session selected by the use
		ZeroMemory(&sessionDesc, sizeof(DPSESSIONDESC2));
		sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
		//		sessionDesc.guidApplication = OVERRIDE_GUID;
		sessionDesc.guidApplication = c->guidApplication;
		sessionDesc.guidInstance = statusContext.guidInstance;
		
		
		// open it
		AttemptOpen = MAXOPENTIMEOUTS;
		hr = DPERR_TIMEOUT;
		while (AttemptOpen && hr == DPERR_TIMEOUT)
		{
			hr = lpDPlay->lpVtbl->Open(lpDPlay, &sessionDesc, DPOPEN_JOIN);
			AttemptOpen--;
		}
		
		if (hr == DP_OK || hr == DPERR_ALREADYINITIALIZED ) /* gfg */
			c->status = LDPLAY_OPENED;
		else
			goto FAILURE;
		
		
#ifdef THREADED
		hr = lpDPlay->lpVtbl->CreatePlayer(lpDPlay, &c->dpidPlayer,NULL,c->hPlayerEvent,NULL,0,0);
#else
		hr = lpDPlay->lpVtbl->CreatePlayer(lpDPlay, &c->dpidPlayer,NULL,NULL,NULL,0,0);
#endif
		
		if (hr == DP_OK)
		{
			c->status = LDPLAY_PLAYER_CREATED;
			c->state  = COMAPI_STATE_CONNECTED;
		}
		
		hr = lpDPlay->lpVtbl->EnumPlayers(lpDPlay, NULL,EnumPlayersCallback2,
			(LPVOID)c,DPENUMPLAYERS_REMOTE);
		
		if FAILED(hr)
			goto FAILURE;
		
		
		
		/*      c->connect_callback_func((ComAPIHandle)c,hr);  */
		/* c->dpidPlayerRemote = 1;   */
		
		
		
		
	}  /* end JOINer */
	
	
	
	
	if(c->close_status != LDPLAY_OK_TO_CLOSE)
	{
		
		
		/*DPGETCAPS_GUARANTEED*/
		DPCaps.dwSize = sizeof(DPCAPS);
		hr = lpDPlay->lpVtbl->GetCaps(lpDPlay,&DPCaps,DPGETCAPS_GUARANTEED);
		if FAILED(hr)
			goto FAILURE;
		
		
		if ((DPCaps.dwFlags & DPCAPS_GUARANTEEDSUPPORTED) || (DPCaps.dwFlags & DPCAPS_GUARANTEEDOPTIMIZED))
		{
			c->sendFlags = DPSEND_GUARANTEED; 
		}
		else
			c->sendFlags = 0; 
		
		// return the connected interface
		c->lpDPlay = lpDPlay;
	}
	
	// set to NULL so we don't release it below
	lpDPlay = NULL;
	
FAILURE:
	
	if (lpDPlay)   /* NON NULL MEANS we FAILED */
	{
		if (!c->bIsHost) /* call for Joiner only */
		{
			c->connect_callback_func(NULL,hr); 
		}
        
		
		c->closer = COMMS;
		ComDPLAYClose((ComAPIHandle)c);
		c = NULL;
	}
	
	
	
	if (lpAddress)
		free(lpAddress);
	if (c)
	{
		if (!c->bIsHost) /* call for Joiner only */
		{
			c->connect_callback_func((ComAPIHandle)c,hr); 
		}
	}
	
	doing_host_or_join = FALSE;
	
	return hr;
}

// ---------------------------------------------------------------------------
// EnumModemAddress
// ---------------------------------------------------------------------------
// Description:             Enumeration callback called by DirectPlayLobby.
//							Enumerates the DirectPlay address chunks. If the
//							chunk contains modem strings, add them to the control.
// Arguments:
//  REFGUID                 [in] GUID of the address type
//  DWORD					[in] size of chunk
//  LPVOID				    [in] pointer to chunk
//  LPVOID				    [in] user-defined context
// Returns:
//  BOOL					FALSE to stop enumerating after the first callback
BOOL FAR PASCAL EnumModemAddress(REFGUID lpguidDataType, DWORD dwDataSize,
							LPCVOID lpData, LPVOID lpContext)
{

		LPSTR	lpszStr = (LPSTR) lpData;
		ComDPLAY *c = (ComDPLAY *)lpContext;

		// modem
		if (IsEqualGUID(lpguidDataType, &DPAID_Modem))
		{
			// loop over all strings in list
			while (lstrlen(lpszStr))
			{
				// store modem name in combo box
				if(c->modemName == NULL)
				{
					c->modemName = malloc(dwDataSize);
					lstrcpy(c->modemName,lpszStr);
				}

				// skip to next string
				lpszStr += lstrlen(lpszStr) + 1;
			}
		}

		return TRUE;
}

// ---------------------------------------------------------------------------
// GetModemnames    
// ---------------------------------------------------------------------------
// Description:             Fills combo box with modem names
// Arguments:
//  HWND                    [in]  Window handle.
//  LPDIRECTPLAYLOBBY2A     [in]  DirectPlay Lobby interface to use
//  LPGUID					[out] GUID of service provider to use
// Returns:
//  HRESULT					any error
HRESULT GetModemNames( LPDIRECTPLAYLOBBY2A lpDPlayLobby, ComDPLAY *c)
{
	LPDIRECTPLAY		lpDPlay1 = NULL;
	LPDIRECTPLAY3A		lpDPlay3A = NULL;
	LPVOID				lpAddress = NULL;
	DWORD				dwAddressSize = 0;
	GUID				guidServiceProvider = DPSPGUID_MODEM;
	HRESULT				hr;

	// get a DirectPlay interface for this service provider
	hr = CAPI_DirectPlayCreate(&guidServiceProvider, &lpDPlay1, NULL);
	if FAILED(hr)
		goto FAILURE;

	// query for an ANSI DirectPlay3 interface
	hr = lpDPlay1->lpVtbl->QueryInterface(lpDPlay1, &IID_IDirectPlay3A, (LPVOID *) &lpDPlay3A);
	if FAILED(hr)
		goto FAILURE;

	// get size of player address for player zero
	hr = lpDPlay3A->lpVtbl->GetPlayerAddress(lpDPlay3A, DPID_ALLPLAYERS, NULL, &dwAddressSize);
	if (hr != DPERR_BUFFERTOOSMALL)
		goto FAILURE;

	// make room for it
	lpAddress = malloc( dwAddressSize);
	if (lpAddress == NULL)
	{
		hr = DPERR_NOMEMORY;
		goto FAILURE;
	}

	// get the address
	hr = lpDPlay3A->lpVtbl->GetPlayerAddress(lpDPlay3A, DPID_ALLPLAYERS, lpAddress, &dwAddressSize);
	if FAILED(hr)
		goto FAILURE;

	// get modem strings from address and put them in the combo box
	hr = lpDPlayLobby->lpVtbl->EnumAddress(lpDPlayLobby, EnumModemAddress, 
							 lpAddress, dwAddressSize, (LPVOID) c);
	if FAILED(hr)
		goto FAILURE;


FAILURE:
	if (lpDPlay1)
		lpDPlay1->lpVtbl->Release(lpDPlay1);
	if (lpDPlay3A)
		lpDPlay3A->lpVtbl->Release(lpDPlay3A);
	if (lpAddress)
		free(lpAddress);

	return (hr);
}



/* get the associated write buffer */
char *ComDPLAYSendBufferGet(ComAPIHandle c)
{
  if(c)
  {
     return ((ComDPLAY *)c)->send_buffer;
  }
  else return NULL;
}


/* get the assocated read buffer */
char *ComDPLAYRecvBufferGet(ComAPIHandle c)
{
  if(c)
  {
     return ((ComDPLAY *)c)->recv_buffer;
  }
  else
	  return NULL;
}


#ifdef THREADED

DWORD WINAPI ReceiveThread(LPVOID lpThreadParameter)
{
    ComDPLAY	   *lpDPInfo = (ComDPLAY *) lpThreadParameter;
	HANDLE		eventHandles[2];

	eventHandles[0] = lpDPInfo->hPlayerEvent;
	eventHandles[1] = lpDPInfo->ghKillReceiveEvent;

	// loop waiting for player events. If the kill event is signaled
	// the thread will exit
	while (WaitForMultipleObjects(2, eventHandles, FALSE, INFINITE) == WAIT_OBJECT_0)
	{
		// receive any messages in the queue
		ReceiveMessage(lpDPInfo);
	}

	ExitThread(0);

	return (0);
}

int ReceiveMessage(ComDPLAY *lpDPInfo)
{
	DPID				idFrom, idTo;
	LPVOID			lpvMsgBuffer;
	DWORD				dwMsgBufferSize;
	HRESULT			hr;

	lpvMsgBuffer = NULL;
	dwMsgBufferSize = 0;

	// loop to read all messages in queue
	do
	{
		// loop until a single message is successfully read
		do
		{
			// read messages from any player, including system player
			idFrom = 0;
			idTo = 0;

	        dwMsgBufferSize = lpDPInfo->buffer_size;                                                                  

			hr = lpDPInfo->lpDPlay->lpVtbl->Receive(lpDPInfo->lpDPlay,&idFrom, &idTo, DPRECEIVE_ALL,
												   lpDPInfo->recv_buffer, &dwMsgBufferSize);


         if(hr != DPERR_NOMESSAGES && hr != DP_OK && hr != DPERR_BUFFERTOOSMALL)
		 {
		 }

		// not enough room, so resize buffer
/*
			if (hr == DPERR_BUFFERTOOSMALL)
			{
				if (lpvMsgBuffer)
					free(lpvMsgBuffer);
				lpvMsgBuffer = malloc(dwMsgBufferSize);
				if (lpvMsgBuffer == NULL)
					hr = DPERR_OUTOFMEMORY;
			}
*/
		} while (hr == DPERR_BUFFERTOOSMALL);


		if ((SUCCEEDED(hr)) &&							// successfully read a message
			(dwMsgBufferSize >= sizeof(DPMSG_GENERIC)))	// and it is big enough
		{
			// check for system message
			if (idFrom == DPID_SYSMSG)
			{
				HandleSystemMessage(lpDPInfo, (LPDPMSG_GENERIC) lpDPInfo->recv_buffer,
									dwMsgBufferSize, idFrom, idTo);
			}
			else
			{
				lpDPInfo->recvmessagecount++;
				hr = dwMsgBufferSize;
                lpDPInfo->messagesize = dwMsgBufferSize;

#ifdef TIMESTAMP
				if(CAPI_TimeStamp)
					lpDPInfo->timestamp = CAPI_TimeStamp();
#endif



			}
		}
	} while (SUCCEEDED(hr));

	// free any memory we created
//	if (lpvMsgBuffer)
//		free(lpvMsgBuffer);

	return (hr);
}

int ComDPLAYRecv(ComAPIHandle c)
{
		ComDPLAY *lpDPInfo = (ComDPLAY *)c;

        int len;
        len = lpDPInfo->messagesize;
        lpDPInfo->messagesize = 0;
        return len;
}

#else

/* Non threaded receive - much better than all that asychronous windows nightmare */

int ComDPLAYRecv(ComAPIHandle c)
{
		DPID				idFrom, idTo;
		LPVOID			lpvMsgBuffer;
		DWORD				dwMsgBufferSize;
		HRESULT			hr;
		ComDPLAY *lpDPInfo = (ComDPLAY *)c;

		lpvMsgBuffer = NULL;
		dwMsgBufferSize = 0;

		// read messages from any player, including system player
		idFrom = 0;
		idTo = 0;

		dwMsgBufferSize = lpDPInfo->buffer_size;                                                                  
		hr = lpDPInfo->lpDPlay->lpVtbl->Receive(lpDPInfo->lpDPlay,&idFrom, &idTo, DPRECEIVE_ALL,
												   lpDPInfo->recv_buffer, &dwMsgBufferSize);


		if(hr != DPERR_NOMESSAGES && hr != DP_OK )
		{
		}


		if ((SUCCEEDED(hr)) &&							// successfully read a message
			(dwMsgBufferSize >= sizeof(DPMSG_GENERIC)))	// and it is big enough
		{
			// check for system message
			if (idFrom == DPID_SYSMSG)
			{
				hr = HandleSystemMessage(lpDPInfo, (LPDPMSG_GENERIC) lpDPInfo->recv_buffer,
									dwMsgBufferSize, idFrom, idTo);
     
			}
			else
			{
				lpDPInfo->recvmessagecount++;
				hr = dwMsgBufferSize;
#ifdef TIMESTAMP
				if(CAPI_TimeStamp)
					lpDPInfo->timestamp = CAPI_TimeStamp();
#endif
			}
		}

        else
		{
			if (hr == DPERR_NOMESSAGES)
                    hr = 0;
            else if (hr == DPERR_BUFFERTOOSMALL)
                    hr = COMAPI_MESSAGE_TOO_BIG;
		}

    	return (hr);
}


#endif

static count=0;
/* shouldn't need these handler -- will be done inthe application */

int HandleSystemMessage(ComDPLAY *lpDPInfo, LPDPMSG_GENERIC lpMsg, DWORD dwMsgSize,
						 DPID idFrom, DPID idTo)
{
	LPSTR		lpszStr = NULL;
    HRESULT  hr=0;

	dwMsgSize;
	idTo;		 // to get rid of warnings
	idFrom;

    // The body of each case is there so you can set a breakpoint and examine
    // the contents of the message received.
	//	MessageBox(NULL,"HandleSystemMessage"," ",MB_OK);


	switch (lpMsg->dwType)
	{
	case DPSYS_CREATEPLAYERORGROUP:
        {
			LPDPMSG_CREATEPLAYERORGROUP		lp = (LPDPMSG_CREATEPLAYERORGROUP) lpMsg;
			LPSTR							lpszPlayerName;
			LPSTR							szDisplayFormat = "\"%s\" has joined\r\n";


			lpDPInfo->dpidPlayerRemote = lp->dpId;
			lpDPInfo->state  = COMAPI_STATE_ACCEPTED;

			lpDPInfo->connect_callback_func((ComAPIHandle)lpDPInfo,0);

			// get pointer to player name
			if (lp->dpnName.lpszShortNameA)
				lpszPlayerName = lp->dpnName.lpszShortNameA;
			else
				lpszPlayerName = "unknown";

			// allocate space for string
			lpszStr = (LPSTR) malloc(lstrlen(szDisplayFormat) +
												   lstrlen(lpszPlayerName) + 1);
			if (lpszStr == NULL)
				break;

			// build string
			wsprintf(lpszStr, szDisplayFormat, lpszPlayerName);
        }
		break;

	case DPSYS_DESTROYPLAYERORGROUP:
        {
			LPDPMSG_DESTROYPLAYERORGROUP	lp = (LPDPMSG_DESTROYPLAYERORGROUP)lpMsg;
//			LPSTR							szDisplayFormat = "\"%s\" has left\r\n";
            
            
			lpDPInfo->dpidPlayerRemote = 0;
			hr  = COMAPI_PLAYER_LEFT;
        }
		break;

	case DPSYS_ADDPLAYERTOGROUP:
        {
            LPDPMSG_ADDPLAYERTOGROUP lp = (LPDPMSG_ADDPLAYERTOGROUP)lpMsg;
        }
		break;

	case DPSYS_DELETEPLAYERFROMGROUP:
        {
            LPDPMSG_DELETEPLAYERFROMGROUP lp = (LPDPMSG_DELETEPLAYERFROMGROUP)lpMsg;
        }
		break;

	case DPSYS_SESSIONLOST:
        {
            LPDPMSG_SESSIONLOST lp = (LPDPMSG_SESSIONLOST)lpMsg;
        }
		break;

	case DPSYS_HOST:
        {
			LPDPMSG_HOST	lp = (LPDPMSG_HOST)lpMsg;
			LPSTR			szDisplayFormat = "You have become the host\r\n";

			// allocate space for string
			lpszStr = (LPSTR) malloc(lstrlen(szDisplayFormat) + 1);
			if (lpszStr == NULL)
				break;

			// build string
			lstrcpy(lpszStr, szDisplayFormat);

			// we are now the host
			lpDPInfo->bIsHost = TRUE;
        }
		break;

	case DPSYS_SETPLAYERORGROUPDATA:
        {
            LPDPMSG_SETPLAYERORGROUPDATA lp = (LPDPMSG_SETPLAYERORGROUPDATA)lpMsg;
        }
		break;

	case DPSYS_SETPLAYERORGROUPNAME:
        {
            LPDPMSG_SETPLAYERORGROUPNAME lp = (LPDPMSG_SETPLAYERORGROUPNAME)lpMsg;
        }
		break;
	}

    if(lpszStr)
        free(lpszStr);

   return   hr;
}


int ComDPLAYSendX(ComAPIHandle c, int msgsize, ComAPIHandle Xcom)
{
		if(c == Xcom) return 0;
		else return   ComDPLAYSend( c, msgsize, FALSE);


}


void ComAPIDPLAYSendMode(ComAPIHandle comhandle, int sendmode)
{
	ComDPLAY *c = (ComDPLAY *)comhandle;

    if(comhandle)
    {
        if(comhandle->protocol == CAPI_DPLAY_TCP_PROTOCOL || comhandle->protocol == CAPI_DPLAY_MODEM_PROTOCOL
            || comhandle->protocol == CAPI_DPLAY_SERIAL_PROTOCOL)
        {
            if(sendmode == CAPI_DPLAY_GUARANTEED)
                c->sendFlags = DPSEND_GUARANTEED;
            else
                c->sendFlags = 0;
        }
    }

}

//static clock_t sendtime0, sendtime1, total=0, scounter=0;


int ComDPLAYSend(ComAPIHandle comhandle, int msgsize, int oob)
{
  
		HRESULT hr;
		ComDPLAY *c = (ComDPLAY *)comhandle;

		oob;

		if (msgsize == 0)
		{
			return 0;
		}

		// send this string to all other players
//        sendtime0 = clock();
		hr = c->lpDPlay->lpVtbl->Send(c->lpDPlay,c->dpidPlayer,c->dpidPlayerRemote,
											c->sendFlags, c->send_buffer, msgsize);
		if(hr == DP_OK)
		{
			use_bandwidth(msgsize, 0);
			c->sendmessagecount++;
			hr = msgsize;
		}
		else if(hr == DPERR_INVALIDPARAMS || hr  == DPERR_INVALIDPLAYER)
		{

			hr = COMAPI_CONNECTION_CLOSED;
		}
		else  if(hr == DPERR_BUSY)
		{
			hr =  COMAPI_WOULDBLOCK;
		}

		else 
		{
		}
			return hr;


}

unsigned long ComDPLAYQuery(ComAPIHandle c, int querytype)
{
	if(c)
	{
   
      switch(querytype)
      {
        case COMAPI_MESSAGECOUNT:
          return ((ComDPLAY *)c)->sendmessagecount + ((ComDPLAY *)c)->recvmessagecount;
		    break;
        case COMAPI_SEND_MESSAGECOUNT:
          return ((ComDPLAY *)c)->sendmessagecount;
		    break;
        case COMAPI_RECV_MESSAGECOUNT:
          return ((ComDPLAY *)c)->recvmessagecount;
		    break;
        case COMAPI_RECV_WOULDBLOCKCOUNT:
          return ((ComDPLAY *)c)->recvwouldblockcount;
		    break;
        case COMAPI_SEND_WOULDBLOCKCOUNT:
          return ((ComDPLAY *)c)->sendwouldblockcount;
		    break;
        case COMAPI_RELIABLE:
          return 1;
		    break;
        case COMAPI_MAX_BUFFER_SIZE:
          return  0;
		    break;
        case COMAPI_ACTUAL_BUFFER_SIZE:
          return ((ComDPLAY *)c)->buffer_size;
		    break;
         case COMAPI_PROTOCOL:
           return  c->protocol;
           break;
         case COMAPI_STATE:
           return ((ComDPLAY *)c)->state;
           break;

         case  COMAPI_SENDER:
		 case  COMAPI_DPLAY_REMOTEPLAYERID:
           return ((ComDPLAY *)c)->dpidPlayerRemote;
           break;

		 case  COMAPI_DPLAY_PLAYERID:
           return ((ComDPLAY *)c)->dpidPlayer;
           break;
         case COMAPI_CONNECTION_ADDRESS:
           return (ComAPIinet_haddr(((ComDPLAY *)c)->IPAddress));
           break;
                      

       default:
	     return 0;

      }
	}
    return 0;

}




#define COMMAXSTRING 100
static char comerror[COMMAXSTRING];
char *ComErrorString(HRESULT hr)
{
   char *msg;
   msg = comerror;

   *msg = 0;                           /* clear message buffer */
   if (hr != DP_OK) 
	   sprintf(msg,"ERROR:");

	switch(hr)
    {

    case DP_OK:
		  strcat(msg,"DP_OK");
		  break;
	 case DPERR_ALREADYINITIALIZED:
		  strcat(msg,"DPERR_ALREADYINITIALIZED");
		  break;
	 case DPERR_INVALIDFLAGS: 
		  strcat(msg,"DPERR_INVALIDFLAGS");
		  break;
	 case DPERR_INVALIDPARAMS: 
		  strcat(msg,"DPERR_INVALIDPARAMS");
		  break;
    case DPERR_UNAVAILABLE: 
		  strcat(msg,"DPERR_UNAVAILABLE");
		  break;
    case DPERR_CONNECTING: 
 		  strcat(msg,"DPERR_CONNECTING");
  	     break;
    case DPERR_EXCEPTION :
        strcat(msg,"DPERR_EXCEPTION");
  	     break;
    case DPERR_GENERIC :
        strcat(msg,"DPERR_GENERIC");
  	     break;
    case DPERR_INVALIDOBJECT :
        strcat(msg,"DPERR_INVALIDOBJECT");
  	     break;
    case DPERR_UNINITIALIZED :
        strcat(msg,"DPERR_UNINITIALIZED");
  	     break;
    case DPERR_USERCANCEL :
        strcat(msg,"DPERR_USERCANCEL");
  	     break;
    case DPERR_BUFFERTOOSMALL:
        strcat(msg,"DPERR_BUFFERTOOSMALL");
  	     break;
    case DPERR_INVALIDPLAYER:
        strcat(msg,"DPERR_INVALIDPLAYER");
  	     break;
    case DPERR_NOMESSAGES:
        strcat(msg,"DPERR_NOMESSAGES");
        break;
    case DPERR_CANTADDPLAYER:
        strcat(msg,"DPERR_CANTADDPLAYER");
        break;
    case DPERR_CANTCREATEPLAYER: 
       strcat(msg,"DPERR_CANTCREATEPLAYER");
       break;
      case DPERR_NOCONNECTION:
         strcat(msg,"DPERR_NOCONNECTION");
         break;
      case DPERR_ACCESSDENIED:          
         strcat(msg,"DPERR_ACCESSDENIED");
         break;
      case DPERR_ACTIVEPLAYERS:         
         strcat(msg,"DPERR_ACTIVEPLAYERS");
         break;
      case DPERR_CANTCREATEGROUP:       
         strcat(msg,"DPERR_CANTCREATEGROUP");
         break;
      case DPERR_CANTCREATESESSION:     
         strcat(msg,"DPERR_CANTCREATESESSION");
         break;
      case DPERR_CAPSNOTAVAILABLEYET:   
         strcat(msg,"DPERR_CAPSNOTAVAILABLEYET");
         break;
      case DPERR_INVALIDGROUP:          
         strcat(msg,"DPERR_INVALIDGROUP");
         break;
      case DPERR_NOCAPS:                
         strcat(msg,"DPERR_NOCAPS");
         break;
      case DPERR_NOMEMORY:              
         strcat(msg,"DPERR_NOMEMORY");
         break;
      case DPERR_NONAMESERVERFOUND:     
         strcat(msg,"DPERR_NONAMESERVERFOUND");
         break;
      case DPERR_NOPLAYERS:             
         strcat(msg,"DPERR_NOPLAYERS");
         break;
      case DPERR_NOSESSIONS:            
         strcat(msg,"DPERR_NOSESSIONS");
         break;
      case DPERR_PENDING:               
         strcat(msg,"DPERR_PENDING");
         break;
      case DPERR_SENDTOOBIG:            
         strcat(msg,"DPERR_SENDTOOBIG");
         break;
      case DPERR_TIMEOUT:               
         strcat(msg,"DPERR_TIMEOUT");
         break;
      case DPERR_UNSUPPORTED:           
         strcat(msg,"DPEER_UNSUPPORTED");
         break;
      case DPERR_BUSY:                  
         strcat(msg,"DPERR_BUSY");
         break;
      case DPERR_NOINTERFACE:           
         strcat(msg,"DPERR_NOINTERFACE");
         break;
      case DPERR_CANNOTCREATESERVER:    
         strcat(msg,"DPERR_CANNOTCREATESERVER");
         break;
      case DPERR_PLAYERLOST :           
         strcat(msg,"DPERR_PLAYERLOST");
         break;
      case DPERR_SESSIONLOST:           
         strcat(msg,"DPERR_SESSIONLOST");
         break;
       
      case DPERR_NONEWPLAYERS:          
         strcat(msg,"DPERR_NONEWPLAYERS");
         break;
      case DPERR_INVALIDPASSWORD:       
         strcat(msg,"DPERR_INVALIDPASSWORD");
         break;
     
                                  
                                  
      case DPERR_BUFFERTOOLARGE:        
         strcat(msg,"DPERR_BUFFERTOOLARGE");
         break;
      case DPERR_CANTCREATEPROCESS:     
         strcat(msg,"DPERR_CANTCREATEPROCESS");
         break;
      case DPERR_APPNOTSTARTED:         
         strcat(msg,"DPERR_APPNOTSTARTED");
         break;
      case DPERR_INVALIDINTERFACE:      
         strcat(msg,"DPERR_INVALIDINTERFACE");
         break;
      case DPERR_NOSERVICEPROVIDER:     
         strcat(msg,"DPERR_NOSERVICEPROVIDER");
         break;
      case DPERR_UNKNOWNAPPLICATION:    
         strcat(msg,"DPERR_UNKNOWNAPPLICATION");
         break;
      case DPERR_NOTLOBBIED:            
         strcat(msg,"DPERR_NOTLOBBIED");
         break;
      case DPERR_SERVICEPROVIDERLOADED: 
         strcat(msg,"DPERR_SERVICEPROVIDERLOADED");
         break;
      case DPERR_ALREADYREGISTERED:     
         strcat(msg,"DPERR_ALREADYREGISTERED");
         break;
      case DPERR_NOTREGISTERED:         
         strcat(msg,"DPERR_NOTREGISTERED");
         break;
      case DPERR_AUTHENTICATIONFAILED:  
         strcat(msg,"DPERR_AUTHENTICATIONFAILED");
         break;
      case DPERR_CANTLOADSSPI:          
         strcat(msg,"DPERR_CANTLOADSSPI");
         break;
      case DPERR_ENCRYPTIONFAILED :     
         strcat(msg,"DPERR_ENCRYPTIONFAILED");
         break;
      case DPERR_SIGNFAILED :           
         strcat(msg,"DPERR_SIGNFAILED");
         break;
      case DPERR_CANTLOADSECURITYPACKAGE:
         strcat(msg,"DPERR_CANTLOADSECURITYPACKAGE");
         break;
      case DPERR_ENCRYPTIONNOTSUPPORTED:
         strcat(msg,"DPERR_ENCRYPTIONNOTSUPPORTED");
         break;
      case DPERR_CANTLOADCAPI:          
         strcat(msg,"DPERR_CANTLOADCAPI");
         break;
      case DPERR_NOTLOGGEDIN:           
         strcat(msg,"DPERR_NOTLOGGEDIN");
         break;
      case DPERR_LOGONDENIED:           
         strcat(msg,"DPERR_LOGONDENIED");
         break;
      case COMAPI_CONNECTION_TIMEOUT:
         strcat(msg,"COMAPI_CONNECTION_TIMEOUT");
         break;


	default:
	    strcat(msg,"UNKNOWN");
		break;

	}

   return msg;
}


int     ComDPLAYHostIDGet(ComAPIHandle comhandle, char *buf, int reset)
{
    ComDPLAY *c = (ComDPLAY *)comhandle;

    memcpy(buf,&c->dpidPlayer,sizeof(DPID));
    return  0;

  
}


int ComDPLAYSendFromGroup(ComAPIHandle com, int msgsize,char *group_send_buffer)
{

    ComDPLAY *this_cdplay;
    char   *save_send_buffer;
    int ret =0;
	
    this_cdplay = (ComDPLAY *)com;

    if(this_cdplay)
    {
		save_send_buffer = this_cdplay->send_buffer;
		this_cdplay->send_buffer = group_send_buffer;

		if (com->send_func && !IsBadCodePtr((FARPROC) com->send_func)) // JB 010223 CTD
		{
			ret = com->send_func(com, msgsize, FALSE);
		}

		this_cdplay->send_buffer = save_send_buffer ;
    }

    return ret;
}


int ComDPLAYEnumProtocols(int *protocols, int maxprotocols)
{
   ComDPLAYENUM  *com;
   int count=0;

   if(!LoadDLLs()) return 0;

   com = malloc(sizeof(ComDPLAYENUM));
   if(com)
   {
     com->apiheader.protocol = ENUMERATE_PROTOCOLS;
     com->protocols = protocols;
     com->maxprotocols = maxprotocols;
     com->count = 0;


     if (!COMMS_COINITIALIZED)
       CAPI_CoInitialize(NULL);

     CAPI_DirectPlayEnumerate(DirectPlayEnumerateCallback, com);

     if (!COMMS_COINITIALIZED)
     {
        CAPI_CoUninitialize();
        UnLoadDLLs();
     }
     count = com->count;
     free(com);
   }
   return count;

}


static void  ComSetSerialData(LPDPCOMPORTADDRESS ComPortAddress,char *datastring)
{
      char *param;
      char *string;


      /* set  defaults */
      ComPortAddress->dwComPort     = 1;
      ComPortAddress->dwBaudRate    = CBR_56000;
      ComPortAddress->dwStopBits    = ONESTOPBIT;
      ComPortAddress->dwParity      = NOPARITY;
      ComPortAddress->dwFlowControl = DPCPA_RTSDTRFLOW;




      if(strlen(datastring) == 0) return;

      /* determine Com Port */
      string = (char*)malloc(strlen(datastring) + 1);
      strcpy(string,datastring);
      _strupr(string);

      param = strtok(string," ,:");
      do
      {
      
		
         if     (!strcmp(param,"COM1"))
                                            ComPortAddress->dwComPort     = 1;
         else if(!strcmp(param,"COM2"))
                                            ComPortAddress->dwComPort     = 2;
         else if(!strcmp(param,"COM3"))
                                            ComPortAddress->dwComPort     = 3;
         else if(!strcmp(param,"COM4"))
                                            ComPortAddress->dwComPort     = 4;

        
         else if(!strcmp(param,"CBR_9600"))
                                            ComPortAddress->dwBaudRate     = CBR_9600;
         else if(!strcmp(param,"CBR_14400"))
                                            ComPortAddress->dwBaudRate     = CBR_14400;
         else if(!strcmp(param,"CBR_19200"))
                                            ComPortAddress->dwBaudRate     = CBR_19200;
         else if(!strcmp(param,"CBR_38400"))
                                            ComPortAddress->dwBaudRate     = CBR_38400;
         else if(!strcmp(param,"CBR_56000"))
                                            ComPortAddress->dwBaudRate     = CBR_56000;
         else if(!strcmp(param,"CBR_57600"))
                                            ComPortAddress->dwBaudRate     = CBR_57600;
         else if(!strcmp(param,"CBR_115200"))
                                            ComPortAddress->dwBaudRate     = CBR_115200;
         else if(!strcmp(param,"CBR_128000"))
                                            ComPortAddress->dwBaudRate     = CBR_128000;
         else if(!strcmp(param,"CBR_256000"))
                                            ComPortAddress->dwBaudRate     = CBR_256000;


         else if(!strcmp(param,"ONESTOPBIT"))
                                            ComPortAddress->dwStopBits     = ONESTOPBIT;
         else if(!strcmp(param,"ONE5STOPBITS"))
                                            ComPortAddress->dwStopBits     = ONE5STOPBITS;
         else if(!strcmp(param,"TWOSTOPBITS"))
                                            ComPortAddress->dwStopBits     = TWOSTOPBITS;

         else if(!strcmp(param,"NOPARITY"))
                                            ComPortAddress->dwParity       = NOPARITY;
         else if(!strcmp(param,"ODDPARITY"))
                                            ComPortAddress->dwParity       = ODDPARITY;
         else if(!strcmp(param,"EVENPARITY"))
                                            ComPortAddress->dwParity       = EVENPARITY;
         else if(!strcmp(param,"MARKPARITY"))
                                            ComPortAddress->dwParity       = MARKPARITY;


         else if(!strcmp(param,"DPCPA_DTRFLOW"))
                                            ComPortAddress->dwFlowControl   = DPCPA_DTRFLOW;
         else if(!strcmp(param,"DPCPA_NOFLOW"))
                                            ComPortAddress->dwFlowControl   = DPCPA_NOFLOW;
         else if(!strcmp(param,"DPCPA_RTSDTRFLOW"))
                                            ComPortAddress->dwFlowControl   = DPCPA_RTSDTRFLOW;
         else if(!strcmp(param,"DPCPA_RTSFLOW"))
                                            ComPortAddress->dwFlowControl   = DPCPA_RTSFLOW;
         else if(!strcmp(param,"DPCPA_XONXOFFFLOW"))
                                            ComPortAddress->dwFlowControl   = DPCPA_XONXOFFFLOW;





      } while ( param = strtok(NULL," ,:") );

      	if(string)
		free(string);



}



