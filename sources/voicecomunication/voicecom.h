#include "f4vu.h"
#include "fsound.h"
#include "falcsnd\voicemanager.h"
#include "CmpClass.h"
#include "flight.h"
#include "sim\include\simdrive.h"
#define _WIN32_DCOM
#include <stdio.h>
#include <dplay8.h>
#include <dplobby8.h>
#include <dvoice.h>
#include "debuggr.h"

#include "sim\include\navsystem.h"
#include <tchar.h>

//-----------------------------------------------------------------------------
// Function-prototypes
//-----------------------------------------------------------------------------
HRESULT WINAPI DirectPlayMessageHandler(PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer);
HRESULT WINAPI LobbyAppMessageHandler(PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer);
HRESULT WINAPI DirectVoiceServerMessageHandler(PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer);
HRESULT WINAPI DirectVoiceClientMessageHandler(PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer);

BOOL    IsServiceProviderValid(const GUID* pGuidSP);
void StopVoice ();
HRESULT InitDirectPlay();
HRESULT InitDirectPlayVoice();
HRESULT CreateDeviceAddress();
HRESULT CreateHostAddress(WCHAR* pwszHost);
HRESULT HostSession();
HRESULT EnumDirectPlayHosts(char* ip);
HRESULT ConnectToSession();
HRESULT SendDirectPlayMessage();
HRESULT Register();
HRESULT UnRegister();
HRESULT LobbyLaunch();
HRESULT TestDirectVoice();
HWND    GetConsoleHwnd();
void    CleanupDirectPlay();
int main(char* ip);
void CreateGroup(unsigned long freq);
void SetListenFreqsHost (DPNID playerid,unsigned long com1,unsigned long com2,unsigned long guard);
void SetListenFreqsClient (unsigned long com1,unsigned long com2,unsigned long guard);
void TransmistoFreq (unsigned long freq);
void Transmit(int com);
void RefreshVoiceFreqs(void);
void startupvoice (char*);

void DirectVoiceSetVolume(int Channel=0); // only 1 channel // MLR 1/29/2004 - 


/*VOID DXUtil_ConvertWideStringToGeneric( TCHAR* tstrDestination, const WCHAR* wstrSource, int cchDestChar = -1 );
VOID DXUtil_ConvertWideStringToAnsi( CHAR* strDestination, const WCHAR* wstrSource, int cchDestChar = -1 );
*/
//-----------------------------------------------------------------------------
// Miscellaneous helper functions
//-----------------------------------------------------------------------------
/*#define SAFE_DELETE(p)          {if(p) {delete (p);     (p)=NULL;}}
#define SAFE_DELETE_ARRAY(p)    {if(p) {delete[] (p);   (p)=NULL;}}
#define SAFE_RELEASE(p)         {if(p) {(p)->Release(); (p)=NULL;}}
*/
#define USER_HOST       1
#define USER_CONNECT    2
#define USER_EXIT       1
#define USER_SEND       2

#define CMDLINE_REGISTER    "register"
#define CMDLINE_UNREGISTER  "unregister"
