//----------------------------------------------------------------------------
// Voice.cpp  -- STUBBED.
//
// The original implemented multiplayer voice over DirectPlay8 + DirectPlay Voice
// (dplay8.h / dplobby8.h / dvoice.h). These APIs are COMPLETELY removed from the modern
// Windows SDK, so the voice subsystem is disabled: below are no-op implementations of all
// public functions (prototypes in voicecom.h) and global pointers = NULL,
// which other modules extern (F4Comms.cpp: g_pDPClient/g_pDPServer;
// commands.cpp: g_pVoiceClient).
//
// TODO: if multiplayer voice is needed, rewrite on a modern stack
// (e.g. GameNetworkingSockets/Opus) -- a separate large track.
//----------------------------------------------------------------------------
#include <windows.h>
#include "voicecom.h" // public prototypes + DPNID

//-----------------------------------------------------------------------------
// Global DP8/DVoice pointers. Types are forward-declared (interfaces from
// the removed headers) -- needed only to define the pointers that
// other TUs extern. Values are always NULL (subsystem disabled).
//-----------------------------------------------------------------------------
struct IDirectPlay8Client;
struct IDirectPlay8Server;
struct IDirectPlay8LobbiedApplication;
struct IDirectPlay8Address;
struct IDirectPlayVoiceClient;
struct IDirectPlayVoiceServer;

IDirectPlay8Client* g_pDPClient = NULL;
IDirectPlay8Server* g_pDPServer = NULL;
IDirectPlay8LobbiedApplication* g_pLobbyApp = NULL;
IDirectPlay8Address* g_pDeviceAddress = NULL;
IDirectPlay8Address* g_pHostAddress = NULL;
IDirectPlayVoiceClient* g_pVoiceClient = NULL;
IDirectPlayVoiceServer* g_pVoiceServer = NULL;

bool stoppingvoice =
    false; // global from the original Voice.cpp (extern'd in F4Comms)

//-----------------------------------------------------------------------------
// No-op implementations of the public API (see voicecom.h).
//-----------------------------------------------------------------------------
HRESULT WINAPI DirectPlayMessageHandler(PVOID, DWORD, PVOID)
{
    return S_OK;
}
HRESULT WINAPI LobbyAppMessageHandler(PVOID, DWORD, PVOID)
{
    return S_OK;
}
HRESULT WINAPI DirectVoiceServerMessageHandler(PVOID, DWORD, PVOID)
{
    return S_OK;
}
HRESULT WINAPI DirectVoiceClientMessageHandler(PVOID, DWORD, PVOID)
{
    return S_OK;
}

BOOL IsServiceProviderValid(const GUID*)
{
    return FALSE;
}
void StopVoice()
{
}
HRESULT InitDirectPlay()
{
    return S_OK;
}
HRESULT InitDirectPlayVoice()
{
    return S_OK;
}
HRESULT CreateDeviceAddress()
{
    return S_OK;
}
HRESULT CreateHostAddress(WCHAR*)
{
    return S_OK;
}
HRESULT HostSession()
{
    return S_OK;
}
HRESULT EnumDirectPlayHosts(char*)
{
    return S_OK;
}
HRESULT ConnectToSession()
{
    return S_OK;
}
HRESULT SendDirectPlayMessage()
{
    return S_OK;
}
HRESULT Register()
{
    return S_OK;
}
HRESULT UnRegister()
{
    return S_OK;
}
HRESULT LobbyLaunch()
{
    return S_OK;
}
HRESULT TestDirectVoice()
{
    return S_OK;
}
HWND GetConsoleHwnd()
{
    return NULL;
}
void CleanupDirectPlay()
{
}
int VoiceMain(char*)
{
    return 0;
} // renamed from `main` (clang reserves that name)

void CreateGroup(unsigned long)
{
}
void SetListenFreqsHost(DPNID, unsigned long, unsigned long, unsigned long)
{
}
void SetListenFreqsClient(unsigned long, unsigned long, unsigned long)
{
}
void TransmistoFreq(unsigned long)
{
}
void Transmit(int)
{
}
void RefreshVoiceFreqs(void)
{
}
void startupvoice(char*)
{
}
void DirectVoiceSetVolume(int /*Channel*/)
{
}
