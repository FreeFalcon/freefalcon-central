#ifndef COMDPLAY_H
#define COMDPLAY_H

/* data structures for COMMS DIRECTPLAY MODEM interface */
#define MAXADDRESS 20
#define MAXSERIALDATA          100
#define LDPLAY_OPENED          1
#define LDPLAY_PLAYER_CREATED  2
#define LDPLAY_WAIT_FOR_PLAYER 3 
#define LDPLAY_OK_TO_CLOSE     4

typedef struct modemheader {
  unsigned long header_base;
  unsigned long size;
  unsigned long inv_size;
  int  header_size;
} modemHeader;


typedef struct comdplayhandle {
  struct comapihandle apiheader;
  int buffer_size;

  LPDIRECTPLAY3A   lpDPlay;            /* IDirectPlay3A interface pointer */
  LPDIRECTPLAYLOBBY2A lpDPlayLobby;     
  GUID             guidInstance;
  GUID             guidApplication;
  GUID             guidServiceProvider;
  HANDLE           hPlayerEvent;       /* player event to use */
  HANDLE           ghKillReceiveEvent;       /* player event to use */
  DPID             dpidPlayer;         /* ID of player created */
  DPID             dpidPlayerRemote;   /* ID of player remote */
  DWORD            sendFlags;
  int              bIsHost;            /* TRUE if hosting */
  char             phonenumber[MAXADDRESS+1];
  char             IPAddress[MAXADDRESS+1];
  char             SerialData[MAXSERIALDATA+1];
  unsigned long sendmessagecount;
  unsigned long recvmessagecount;
  unsigned long sendwouldblockcount;
  unsigned long recvwouldblockcount;

  HANDLE lock;
  HANDLE ghReceiveThread;
  HANDLE ghConnectThread;
  DWORD  gidConnectThread;
  DWORD  gidReceiveThread;
  short ThreadActive;
  short timeoutsecs;
  short status;
  short state;
  short close_status;
  short closer;
  int messagesize;
  int headersize;
  char *recv_buffer_start;
  char *send_buffer;
  char *recv_buffer;
  char *modemName;
  int bytes_needed_for_header;
  int bytes_needed_for_message;
  int bytes_recvd_for_message;
  modemHeader *Header;
  void (*connect_callback_func)(struct comapihandle *c,int retcode);
  void (*accept_callback_func)(struct comapihandle *c);
  unsigned long timestamp;
  } ComDPLAY;


typedef struct comdplayENUMhandle {
  struct comapihandle apiheader;
  int    *protocols;
  int    count;
  int    maxprotocols;

} ComDPLAYENUM;



typedef HRESULT (PASCAL FAR * DPFN_DirectPlayCreate)( LPGUID lpGUID, LPDIRECTPLAY *lplpDP, IUnknown *pUnk);
#ifdef UNICODE
typedef HRESULT (PASCAL FAR * DPFN_DirectPlayEnumerate)( LPDPENUMDPCALLBACK, LPVOID );
typedef HRESULT (PASCAL FAR * DPFN_DirectPlayLobbyCreate)(LPGUID, LPDIRECTPLAYLOBBY *, IUnknown *, LPVOID, DWORD );
#else
typedef HRESULT (PASCAL FAR *  DPFN_DirectPlayEnumerate)( LPDPENUMDPCALLBACKA, LPVOID );
typedef HRESULT (PASCAL FAR *  DPFN_DirectPlayLobbyCreate)(LPGUID, LPDIRECTPLAYLOBBYA *, IUnknown *, LPVOID, DWORD );
#endif /* UNICODE */

DPFN_DirectPlayCreate        CAPI_DirectPlayCreate = NULL;
DPFN_DirectPlayEnumerate     CAPI_DirectPlayEnumerate = NULL;
DPFN_DirectPlayLobbyCreate   CAPI_DirectPlayLobbyCreate = NULL;


typedef HRESULT  (PASCAL FAR * OLEFN_CoInitialize)(LPVOID pvReserved);
typedef VOID  (PASCAL FAR * OLEFN_CoUninitialize)(void);
typedef HRESULT (PASCAL FAR * OLEFN_CoCreateInstance)(REFCLSID rclsid, LPUNKNOWN pUnkOuter,
                  DWORD dwClsContext, REFIID riid, LPVOID FAR* ppv);
OLEFN_CoInitialize           CAPI_CoInitialize = NULL;
OLEFN_CoUninitialize         CAPI_CoUninitialize = NULL;
OLEFN_CoCreateInstance       CAPI_CoCreateInstance = NULL;


#endif