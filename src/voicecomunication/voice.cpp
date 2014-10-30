//----------------------------------------------------------------------------
// THIS FreeFalcon DP8 VOICE CODED BY RIK TMF@BIGFOOT.COM //ME123
//-----------------------------------------------------------------------------
//#define INITGUID
#include "voicecom.h"
#include "otwdrive.h"
#include "cpmanager.h"
#include "icp.h"
#include "soundfx.h"
#include "vusessn.h"
#include "ui/include/logbook.h"
#include "falcsess.h"
#include "dxutil.h"
//-----------------------------------------------------------------------------
// App specific structures
//-----------------------------------------------------------------------------
struct HOST_NODE
{
    DPN_APPLICATION_DESC*   pAppDesc;
    IDirectPlay8Address*    pHostAddress;
    WCHAR*                  pwszSessionName;

    HOST_NODE*              pNext;
};
DWORD        LocaldwGroup;                          // Group # that player joined in
DWORD        LocaldwTarget;                         // Group # that player is targeting
//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
IDirectPlay8Client*                 g_pDPClient         = NULL;
IDirectPlay8Server*                 g_pDPServer         = NULL;
IDirectPlay8LobbiedApplication*     g_pLobbyApp         = NULL;
IDirectPlay8Address*                g_pDeviceAddress    = NULL;
IDirectPlay8Address*                g_pHostAddress      = NULL;
IDirectPlayVoiceClient*             g_pVoiceClient      = NULL;
IDirectPlayVoiceServer*             g_pVoiceServer      = NULL;
DPNHANDLE                           g_hLobbyHandle      = NULL;
BOOL                                g_bLobbyLaunched    = FALSE;
BOOL                                g_bHost;
BOOL                                g_bRegister         = FALSE;
BOOL                                g_bUnRegister       = FALSE;
WCHAR*                              g_wszPath           = NULL;
HOST_NODE*                          g_pHostList         = NULL;
CRITICAL_SECTION                    g_csHostList;
DPNID                               g_dpnidLocalPlayer  = 0;
extern bool g_bVoiceCom;


void SendFreqid(DPNID dpnidplayer, DPNID dpnidgroup, unsigned long freq);
#define GAME_MSGID_SetClientListenFreqs             1
#define GAME_MSGID_SetFreqId 2
#define GAME_MSGID_Setg_dpnidLocalPlayer     3
#define GAME_MSGID_ResetFreqarrey     4
//#define GAME_MSGID_   4
struct GAMEMSG_GENERIC
{
    DWORD dwType;
};
struct COM_MESSAGE_SetClientListenFreqs : public GAMEMSG_GENERIC
{
    DWORD  dpnid;
    DWORD  com[3];
    char gamename[30];
};
struct COM_MESSAGE_SetFreqId: public GAMEMSG_GENERIC
{
    DWORD  Freq[2];
};
struct COM_MESSAGE_Setg_dpnidLocalPlayer: public GAMEMSG_GENERIC
{
    DPNID  Id;
};

#define maxfreqs 1000
struct freqarrey
{
    int count;
    unsigned long Freq[maxfreqs][2];
};

// This GUID allows DirectPlay to find other instances of the same game on
// the network.  So it must be unique for every game, and the same for
// every instance of that game.  // {91F570F2-CE88-428a-8025-25A1E4918B44}
GUID g_guidApp = { 0x80e553e6, 0xee58, 0x356f, { 0xe3, 0xf5, 0x34, 0x2b, 0x44, 0x54, 0x77, 0x34 } };
bool g_bconected = false;
extern float g_fdwPortclient ;
extern float g_fdwPorthost;
extern bool g_bLowBwVoice ;
DWORD g_ddwPortclient = (DWORD)g_fdwPortclient;
DWORD g_ddwPorthost = (DWORD)g_fdwPorthost;
freqarrey g_afreqarrey;
int g_itransmitfreq = 0;
bool stoppingvoice = FALSE;
void StopVoice(void)
{
    stoppingvoice = TRUE;
    CleanupDirectPlay();

    //    CoUninitialize();
}
//-----------------------------------------------------------------------------
// Name: InitDirectPlay()
// Desc: Initialize DirectPlay
//----------------------------------------------------------------------------
HRESULT InitDirectPlay()
{
    HRESULT     hr = S_OK;


    g_afreqarrey.count = 0;

    for (int i = 0; i < maxfreqs ; i++)
    {
        g_afreqarrey.Freq[i][0] = 0;
        g_afreqarrey.Freq[i][1] = 0;
    }

    // Create the IDirectPlay8Peer Object
    if (g_bHost)
    {
        if (FAILED(hr = CoCreateInstance(CLSID_DirectPlay8Server, NULL,
                                         CLSCTX_INPROC_SERVER,
                                         IID_IDirectPlay8Server,
                                         (LPVOID*) &g_pDPServer)))
        {
            MonoPrint("Failed Creating the IDirectPlay8Peer Object:  0x%X\n", hr);
            goto LCleanup;
        }
    }
    else
    {
        if (FAILED(hr = CoCreateInstance(CLSID_DirectPlay8Client, NULL,
                                         CLSCTX_INPROC_SERVER,
                                         IID_IDirectPlay8Client,
                                         (LPVOID*) &g_pDPClient)))
        {
            MonoPrint("Failed Creating the IDirectPlay8Peer Object:  0x%X\n", hr);
            goto LCleanup;
        }
    }

    // Create the IDirectPlay8LobbiedApplication Object
    if (FAILED(hr = CoCreateInstance(CLSID_DirectPlay8LobbiedApplication, NULL,
                                     CLSCTX_INPROC_SERVER,
                                     IID_IDirectPlay8LobbiedApplication,
                                     (LPVOID*) &g_pLobbyApp)))
    {
        MonoPrint("Failed Creating the IDirectPlay8LobbiedApplication Object:  0x%X\n", hr);
        goto LCleanup;
    }

    // Init DirectPlay
    if (g_bHost)
    {
        if (FAILED(hr = g_pDPServer->Initialize(NULL, DirectPlayMessageHandler, 0)))
        {
            MonoPrint("Failed Initializing DirectPlay:  0x%X\n", hr);
            goto LCleanup;
        }
    }
    else
    {
        if (FAILED(hr = g_pDPClient->Initialize(NULL, DirectPlayMessageHandler, 0)))
        {
            MonoPrint("Failed Initializing DirectPlay:  0x%X\n", hr);
            goto LCleanup;
        }
    }

    // Init the Lobby interface
    if (FAILED(hr = g_pLobbyApp->Initialize(NULL, LobbyAppMessageHandler, &g_hLobbyHandle, 0)))
    {
        MonoPrint("Failed Initializing Lobby:  0x%X\n", hr);
        goto LCleanup;
    }
    else
        g_bLobbyLaunched = g_hLobbyHandle not_eq NULL;

    // Ensure that TCP/IP is a valid Service Provider
    if (FALSE == IsServiceProviderValid(&CLSID_DP8SP_TCPIP))
    {
        hr = E_FAIL;
        MonoPrint("Failed validating CLSID_DP8SP_TCPIP");
        goto LCleanup;
    }

LCleanup:
    return hr;
}




//-----------------------------------------------------------------------------
// Name: IsServiceProviderValid()
// Desc: Return TRUE if the service provider is valid
//-----------------------------------------------------------------------------
BOOL IsServiceProviderValid(const GUID* pGuidSP)
{
    HRESULT                     hr;
    DPN_SERVICE_PROVIDER_INFO*  pdnSPInfo = NULL;
    DWORD                       dwItems = 0;
    DWORD                       dwSize = 0;

    if (g_bHost)
    {
        hr = g_pDPServer->EnumServiceProviders(&CLSID_DP8SP_TCPIP, NULL, NULL, &dwSize, &dwItems, 0);
    }
    else
    {
        hr = g_pDPClient->EnumServiceProviders(&CLSID_DP8SP_TCPIP, NULL, NULL, &dwSize, &dwItems, 0);
    }

    if (hr not_eq DPNERR_BUFFERTOOSMALL)
    {
        MonoPrint("Failed Enumerating Service Providers:  0x%x\n", hr);
        goto LCleanup;
    }

    pdnSPInfo = (DPN_SERVICE_PROVIDER_INFO*) new BYTE[dwSize];

    if (g_bHost)
    {
        if (FAILED(hr = g_pDPServer->EnumServiceProviders(&CLSID_DP8SP_TCPIP, NULL, pdnSPInfo, &dwSize, &dwItems, 0)))
        {
            MonoPrint("Failed Enumerating Service Providers:  0x%x\n", hr);
            goto LCleanup;
        }
    }
    else
    {
        if (FAILED(hr = g_pDPClient->EnumServiceProviders(&CLSID_DP8SP_TCPIP, NULL, pdnSPInfo, &dwSize, &dwItems, 0)))
        {
            MonoPrint("Failed Enumerating Service Providers:  0x%x\n", hr);
            goto LCleanup;
        }
    }

    // There are no items returned so the requested SP is not available
    if (dwItems == 0)
    {
        hr = E_FAIL;
    }

LCleanup:
    SAFE_DELETE_ARRAY(pdnSPInfo);

    if (SUCCEEDED(hr))
        return TRUE;
    else
        return FALSE;
}




//-----------------------------------------------------------------------------
// Name: DirectPlayMessageHandler
// Desc: Handler for DirectPlay messages.
//-----------------------------------------------------------------------------
HRESULT WINAPI DirectPlayMessageHandler(PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer)
{
    HRESULT     hr = S_OK;
    int i = 0;

    switch (dwMessageId)
    {
        case DPN_MSGID_CREATE_GROUP:
        {
            MonoPrint("voice creategroup");
            //g_afreqarrey
            PDPNMSG_CREATE_GROUP message;
            DWORD dwSize = 0;
            DPN_GROUP_INFO* pdpGroupInfo = NULL;

            message = (PDPNMSG_CREATE_GROUP) pMsgBuffer;
            g_pDPServer->GetGroupInfo(message->dpnidGroup, pdpGroupInfo, &dwSize, 0);

            pdpGroupInfo = (DPN_GROUP_INFO*) new BYTE[ dwSize ];
            ZeroMemory(pdpGroupInfo, dwSize);
            pdpGroupInfo->dwSize = sizeof(DPN_GROUP_INFO);
            g_pDPServer->GetGroupInfo(message->dpnidGroup, pdpGroupInfo, &dwSize, 0);

            ////convert the name to the vuid
            TCHAR freq[sizeof(long) * 8 + 1] ;
            DXUtil_ConvertWideStringToGeneric(freq, pdpGroupInfo->pwszName, sizeof(long) * 8 + 1);
            char *p;
            const char* str = freq;
            unsigned long FreqVUID = strtoul(&str[0], &p, sizeof(long) * 8 + 1);
            //////////////////////////////
            SendFreqid(0, message->dpnidGroup, FreqVUID) ;
            break;
        }

        case DPN_MSGID_ENUM_HOSTS_RESPONSE:
        {
            if (g_bconected) return S_OK;

            MonoPrint("voice ENUM_HOSTS_RESPONSE");
            PDPNMSG_ENUM_HOSTS_RESPONSE     pEnumHostsResponseMsg;
            const DPN_APPLICATION_DESC*     pAppDesc;
            HOST_NODE*                      pHostNode = NULL;
            WCHAR*                          pwszSession = NULL;

            pEnumHostsResponseMsg = (PDPNMSG_ENUM_HOSTS_RESPONSE) pMsgBuffer;
            pAppDesc = pEnumHostsResponseMsg->pApplicationDescription;

            // Insert each host response if it isn't already present
            EnterCriticalSection(&g_csHostList);

            for (pHostNode = g_pHostList; pHostNode; pHostNode = pHostNode->pNext)
            {
                if (pAppDesc->guidInstance == pHostNode->pAppDesc->guidInstance)
                {
                    // This host is already in the list
                    pHostNode = NULL;
                    goto Break_ENUM_HOSTS_RESPONSE;
                }
            }

            // This host session is not in the list then so insert it.
            pHostNode = new HOST_NODE;

            if (pHostNode == NULL)
            {
                goto Break_ENUM_HOSTS_RESPONSE;
            }

            ZeroMemory(pHostNode, sizeof(HOST_NODE));

            // Copy the Host Address
            if (FAILED(pEnumHostsResponseMsg->pAddressSender->Duplicate(&pHostNode->pHostAddress)))
            {
                goto Break_ENUM_HOSTS_RESPONSE;
            }

            pHostNode->pAppDesc = new DPN_APPLICATION_DESC;

            if (pHostNode == NULL)
            {
                goto Break_ENUM_HOSTS_RESPONSE;
            }

            ZeroMemory(pHostNode->pAppDesc, sizeof(DPN_APPLICATION_DESC));
            memcpy(pHostNode->pAppDesc, pAppDesc, sizeof(DPN_APPLICATION_DESC));

            // Null out all the pointers we aren't copying
            pHostNode->pAppDesc->pwszSessionName = NULL;
            pHostNode->pAppDesc->pwszPassword = NULL;
            pHostNode->pAppDesc->pvReservedData = NULL;
            pHostNode->pAppDesc->dwReservedDataSize = 0;
            pHostNode->pAppDesc->pvApplicationReservedData = NULL;
            pHostNode->pAppDesc->dwApplicationReservedDataSize = 0;

            if (pAppDesc->pwszSessionName)
            {
                pwszSession = new WCHAR[wcslen(pAppDesc->pwszSessionName) + 1];

                if (pwszSession)
                {
                    wcscpy(pwszSession, pAppDesc->pwszSessionName);
                }
            }

            pHostNode->pwszSessionName = pwszSession;

            // Insert it onto the front of the list
            pHostNode->pNext = g_pHostList ? g_pHostList->pNext : NULL;
            g_pHostList = pHostNode;
            pHostNode = NULL;

        Break_ENUM_HOSTS_RESPONSE:
            LeaveCriticalSection(&g_csHostList);

            if (pHostNode)
            {
                SAFE_RELEASE(pHostNode->pHostAddress);

                SAFE_DELETE(pHostNode->pAppDesc);

                delete pHostNode;
            }

            break;
        }

        case DPN_MSGID_RECEIVE:
        {
            PDPNMSG_RECEIVE pReceiveMsg;
            pReceiveMsg = (PDPNMSG_RECEIVE)pMsgBuffer;
            GAMEMSG_GENERIC* pMsg;
            pMsg = (GAMEMSG_GENERIC*) pReceiveMsg->pReceiveData;

            switch (pMsg->dwType)
            {
                    // this makes a lokal array if VUIDFREQA vs DPNID's so the voiceclient can set the correct transmittarget
                case GAME_MSGID_ResetFreqarrey:
                {
                    g_afreqarrey.count = 0;

                    for (int i = 0; i < maxfreqs ; i++)
                    {
                        g_afreqarrey.Freq[i][0] = 0;
                        g_afreqarrey.Freq[i][1] = 0;
                    }

                    break;
                }

                case GAME_MSGID_SetFreqId:
                {
                    MonoPrint("voice SetFreqId");
                    COM_MESSAGE_SetFreqId* SetFreqIdMessage;
                    SetFreqIdMessage = (COM_MESSAGE_SetFreqId*) pReceiveMsg->pReceiveData;
                    g_afreqarrey.Freq[g_afreqarrey.count][0] =  SetFreqIdMessage->Freq[0];
                    g_afreqarrey.Freq[g_afreqarrey.count][1] =  SetFreqIdMessage->Freq[1];
                    g_afreqarrey.count++;
                    break;
                }

                // this sets a clients group membership
                case GAME_MSGID_SetClientListenFreqs:
                {
                    MonoPrint("voice SetClientListenFreqs");

                    if ( not g_bHost)
                        break;

                    COM_MESSAGE_SetClientListenFreqs* SetClientListenFreqsMessage;
                    SetClientListenFreqsMessage = (COM_MESSAGE_SetClientListenFreqs*) pReceiveMsg->pReceiveData;
                    SetListenFreqsHost(SetClientListenFreqsMessage->dpnid,
                                       SetClientListenFreqsMessage->com[0],
                                       SetClientListenFreqsMessage->com[1],
                                       SetClientListenFreqsMessage->com[2]);
                    break;
                }

                case GAME_MSGID_Setg_dpnidLocalPlayer:
                {
                    MonoPrint("voice Setg_dpnidLocalPlayer");
                    COM_MESSAGE_Setg_dpnidLocalPlayer* Setg_dpnidLocalPlayer;
                    Setg_dpnidLocalPlayer = (COM_MESSAGE_Setg_dpnidLocalPlayer*) pReceiveMsg->pReceiveData;
                    g_dpnidLocalPlayer = Setg_dpnidLocalPlayer->Id;
                    break;
                }
            }

            break;
        }


        case DPN_MSGID_CREATE_PLAYER:
        {
            MonoPrint("voice CREATE_PLAYER");
            PDPNMSG_CREATE_PLAYER   pCreatePlayerMsg;
            DWORD                   dwSize = 0;
            DPN_PLAYER_INFO*        pdpPlayerInfo = NULL;

            pCreatePlayerMsg = (PDPNMSG_CREATE_PLAYER)pMsgBuffer;

            // check to see if we are the player being created
            if (g_dpnidLocalPlayer)
            {
                hr = g_pDPServer->GetClientInfo(pCreatePlayerMsg->dpnidPlayer, pdpPlayerInfo, &dwSize, 0);
            }
            else
            {
                g_dpnidLocalPlayer = pCreatePlayerMsg->dpnidPlayer;
                return hr;
            }

            if (FAILED(hr) and hr not_eq DPNERR_BUFFERTOOSMALL)
            {
                MonoPrint("Failed GetPeerInfo:  0x%X\n", hr);
                return hr;
            }

            pdpPlayerInfo = (DPN_PLAYER_INFO*) new BYTE[dwSize];
            ZeroMemory(pdpPlayerInfo, dwSize);
            pdpPlayerInfo->dwSize = sizeof(DPN_PLAYER_INFO);

            if (FAILED(hr = g_pDPServer->GetClientInfo(pCreatePlayerMsg->dpnidPlayer, pdpPlayerInfo, &dwSize, 0)))


            {
                MonoPrint("Failed GetPeerInfo:  0x%X\n", hr);
                goto Error_DPN_MSGID_CREATE_PLAYER;
            }

            DPN_BUFFER_DESC dpnBuffer;
            COM_MESSAGE_Setg_dpnidLocalPlayer  comData;

            comData.dwType =  GAME_MSGID_Setg_dpnidLocalPlayer;
            comData.Id = pCreatePlayerMsg->dpnidPlayer;

            dpnBuffer.pBufferData = (BYTE*) &comData;
            dpnBuffer.dwBufferSize = sizeof(COM_MESSAGE_Setg_dpnidLocalPlayer);

            if (FAILED(hr = g_pDPServer->SendTo(pCreatePlayerMsg->dpnidPlayer,    // dpnid
                                                &dpnBuffer,             // pBufferDesc
                                                1,                      // cBufferDesc
                                                0,                      // dwTimeOut
                                                NULL,                   // pvAsyncContext
                                                NULL,                   // pvAsyncHandle
                                                DPNSEND_SYNC bitor DPNSEND_GUARANTEED)))    // dwFlags
            {
                printf("Failed Sending Data:  0x%x\n", hr);
            }


            for (i; i < g_afreqarrey.count; i++)
            {
                SendFreqid(
                    pCreatePlayerMsg->dpnidPlayer,
                    g_afreqarrey.Freq[i][0],
                    g_afreqarrey.Freq[i][1]);
            }

        Error_DPN_MSGID_CREATE_PLAYER:
            SAFE_DELETE_ARRAY(pdpPlayerInfo);
            break;
        }
    }

    return hr;
}




//-----------------------------------------------------------------------------
// Name: LobbyAppMessageHandler
// Desc: Handler for DirectPlay lobby messages
//-----------------------------------------------------------------------------
HRESULT WINAPI LobbyAppMessageHandler(PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer)
{
    HRESULT     hr = S_OK;

    switch (dwMessageId)
    {
        case DPL_MSGID_CONNECT:
        {
            PDPL_MESSAGE_CONNECT        pConnectMsg;
            PDPL_CONNECTION_SETTINGS    pSettings;

            pConnectMsg = (PDPL_MESSAGE_CONNECT)pMsgBuffer;
            pSettings = pConnectMsg->pdplConnectionSettings;

            // Register the lobby with directplay so we get automatic notifications
            if (g_bHost)
                hr = g_pDPServer->RegisterLobby(pConnectMsg->hConnectId, g_pLobbyApp, DPNLOBBY_REGISTER);
            else
                hr = g_pDPClient->RegisterLobby(pConnectMsg->hConnectId, g_pLobbyApp, DPNLOBBY_REGISTER);

            break;
        }
    }

    return hr;
}




//-----------------------------------------------------------------------------
// Name: DirectVoiceServerMessageHandler
// Desc: Handler for DirectPlay Voice server messages
//-----------------------------------------------------------------------------
HRESULT WINAPI DirectVoiceServerMessageHandler(PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer)
{
    if (OTWDriver.pCockpitManager and OTWDriver.pCockpitManager->mpIcp)
        // switch(dwMessageId)
    {

    }

    // In this app we are not responding to any server messages
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DirectVoiceClientMessageHandler
// Desc: Handler for DirectPlay Voice client messages
//-----------------------------------------------------------------------------
HRESULT WINAPI DirectVoiceClientMessageHandler(PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer)
{
    HRESULT     hr = S_OK;

    switch (dwMessageId)
    {
        case DVMSGID_RECORDSTART:
        {
            if ( not OTWDriver.pCockpitManager or not OTWDriver.pCockpitManager->mpIcp) break;

            if (g_itransmitfreq == 1)
            {
                OTWDriver.pCockpitManager->mpIcp->transmitingvoicecom1 = TRUE;
                OTWDriver.pCockpitManager->mpIcp->transmitingvoicecom2 = FALSE;
            }
            else if (g_itransmitfreq == 2)
            {
                OTWDriver.pCockpitManager->mpIcp->transmitingvoicecom1 = FALSE;
                OTWDriver.pCockpitManager->mpIcp->transmitingvoicecom2 = TRUE;
            }

            MonoPrint("Record Start\n");
            break;
        }

        case DVMSGID_RECORDSTOP:
        {
            if ( not OTWDriver.pCockpitManager or not OTWDriver.pCockpitManager->mpIcp) break;

            OTWDriver.pCockpitManager->mpIcp->transmitingvoicecom1 = FALSE;
            OTWDriver.pCockpitManager->mpIcp->transmitingvoicecom2 = FALSE;
            MonoPrint("Record Stop\n");
            break;
        }

        case DVMSGID_PLAYERVOICESTART:
        {
            MonoPrint("play Start\n");
            break;
        }

        case DVMSGID_PLAYERVOICESTOP :
        {
            F4SoundFXSetDist(SFX_MIKECLICK, FALSE, 0.0f, 1.0f);
            MonoPrint("play Stop\n");
            break;
        }
    }

    return hr;
}

void DirectVoiceSetVolume(int Channel) // only 1 channel
{
    // MLR this is a test to see if we can manage the volume of Voice Comms.
    if (g_pVoiceClient)
    {
        DVCLIENTCONFIG cf;
        g_pVoiceClient->GetClientConfig(&cf);
        cf.lPlaybackVolume = PlayerOptions.GroupVol[COM2_SOUND_GROUP]; // how about that MLR 2003-10-20
        g_pVoiceClient->SetClientConfig(&cf);
    }
}



#include <types.h>

//-----------------------------------------------------------------------------
// Name: EnumDirectPlayHosts()
// Desc: Enumerates the hosts
//-----------------------------------------------------------------------------
HRESULT EnumDirectPlayHosts(char* ip)
{
    HRESULT                 hr = S_OK;
    WCHAR                   wszHost[128];
    DPN_APPLICATION_DESC    dpAppDesc;
    WCHAR*                  pwszURL = NULL;

    const char* str = ip;
    mbstowcs(&wszHost[0], str, 128);

    if (FAILED(hr = CreateHostAddress(wszHost)))
    {
        MonoPrint("Failed Creating Host Address:  0x%X\n", hr);
        goto LCleanup;
    }

    // Now set up the Application Description
    ZeroMemory(&dpAppDesc, sizeof(DPN_APPLICATION_DESC));
    dpAppDesc.dwSize = sizeof(DPN_APPLICATION_DESC);
    dpAppDesc.guidApplication = g_guidApp;

    // We now have the host address so lets enum
    if (g_bHost)
    {
        goto LCleanup;
    }
    else
    {
        if (FAILED(hr = g_pDPClient->EnumHosts(&dpAppDesc,             // pApplicationDesc
                                               g_pHostAddress,     // pdpaddrHost
                                               g_pDeviceAddress,   // pdpaddrDeviceInfo
                                               NULL, 0,            // pvUserEnumData, size
                                               4,                  // dwEnumCount
                                               0,                  // dwRetryInterval
                                               0,                  // dwTimeOut
                                               NULL,               // pvUserContext
                                               NULL,               // pAsyncHandle
                                               DPNENUMHOSTS_SYNC)))   // dwFlags
        {
            MonoPrint("Failed Enumerating the Hosts:  0x%X\n", hr);
            goto LCleanup;
        }
    }

LCleanup:
    return hr;
}




//-----------------------------------------------------------------------------
// Name: CreateDeviceAddress()
// Desc: Creates a device address
//-----------------------------------------------------------------------------
HRESULT CreateDeviceAddress()
{
    HRESULT         hr = S_OK;

    // Create our IDirectPlay8Address Device Address
    if (FAILED(hr = CoCreateInstance(CLSID_DirectPlay8Address, NULL,
                                     CLSCTX_INPROC_SERVER,
                                     IID_IDirectPlay8Address,
                                     (LPVOID*) &g_pDeviceAddress)))
    {
        MonoPrint("Failed Creating the IDirectPlay8Address Object:  0x%X\n", hr);
        goto LCleanup;
    }

    // Set the SP for our Device Address
    if (FAILED(hr = g_pDeviceAddress->SetSP(&CLSID_DP8SP_TCPIP)))
    {
        MonoPrint("Failed Setting the Service Provider:  0x%X\n", hr);
        goto LCleanup;
    }

    if (g_bHost and g_ddwPorthost)
    {
        if (FAILED(hr = g_pDeviceAddress->AddComponent(DPNA_KEY_PORT,             //pwszName
                        &g_ddwPorthost, sizeof(g_ddwPorthost),   //lpvData, dwDataSize
                        DPNA_DATATYPE_DWORD)))       //dwDataType
        {
            printf("Failed Adding Hostname to Host port:  0x%X\n", hr);
            goto LCleanup;
        }
    }
    else if ( not g_bHost and g_ddwPortclient)
    {
        if (FAILED(hr = g_pDeviceAddress->AddComponent(DPNA_KEY_PORT,             //pwszName
                        &g_ddwPortclient, sizeof(g_ddwPortclient),   //lpvData, dwDataSize
                        DPNA_DATATYPE_DWORD)))       //dwDataType
        {
            printf("Failed Adding Hostname to Host port:  0x%X\n", hr);
            goto LCleanup;
        }
    }

LCleanup:
    return hr;
}




//-----------------------------------------------------------------------------
// Name: CreateHostAddress()
// Desc: Creates a host address
//-----------------------------------------------------------------------------
HRESULT CreateHostAddress(WCHAR* pwszHost)
{
    HRESULT         hr = S_OK;

    // Create our IDirectPlay8Address Host Address
    if (FAILED(hr = CoCreateInstance(CLSID_DirectPlay8Address, NULL,
                                     CLSCTX_INPROC_SERVER,
                                     IID_IDirectPlay8Address,
                                     (LPVOID*) &g_pHostAddress)))
    {
        MonoPrint("Failed Creating the IDirectPlay8Address Object:  0x%X\n", hr);
        goto LCleanup;
    }

    // Set the SP for our Host Address
    if (FAILED(hr = g_pHostAddress->SetSP(&CLSID_DP8SP_TCPIP)))
    {
        MonoPrint("Failed Setting the Service Provider:  0x%X\n", hr);
        goto LCleanup;
    }

    // Set the hostname into the address
    if (FAILED(hr = g_pHostAddress->AddComponent(DPNA_KEY_HOSTNAME, pwszHost,
                    2 * (wcslen(pwszHost) + 1), /*bytes*/
                    DPNA_DATATYPE_STRING)))
    {
        MonoPrint("Failed Adding Hostname to Host Address:  0x%X\n", hr);
        goto LCleanup;
    }

    if (g_ddwPorthost)
    {
        if (FAILED(hr = g_pHostAddress->AddComponent(DPNA_KEY_PORT,             //pwszName
                        &g_ddwPorthost, sizeof(g_ddwPorthost),   //lpvData, dwDataSize
                        DPNA_DATATYPE_DWORD)))       //dwDataType
        {
            printf("Failed Adding Hostname to Host port:  0x%X\n", hr);
            goto LCleanup;
        }
    }

LCleanup:
    return hr;
}




//-----------------------------------------------------------------------------
// Name: HostSession()
// Desc: Host a DirectPlay session
//-----------------------------------------------------------------------------
HRESULT HostSession()
{
    HRESULT                 hr = S_OK;
    DPN_APPLICATION_DESC    dpAppDesc;
    WCHAR                   wszSession[128];


    // Prompt the user for the session name
    MonoPrint("\nHostign a Voice Session.\n");
    //wscanf(L"%ls", wszSession);
    //wcscpy(wszSession,L "FreeFalcon");
    wcscpy(wszSession, L"FreeFalcon");

    // Now set up the Application Description
    ZeroMemory(&dpAppDesc, sizeof(DPN_APPLICATION_DESC));
    dpAppDesc.dwSize = sizeof(DPN_APPLICATION_DESC);
    dpAppDesc.guidApplication = g_guidApp;
    dpAppDesc.pwszSessionName = wszSession;
    dpAppDesc.dwFlags = DPNSESSION_NODPNSVR bitor DPNSESSION_CLIENT_SERVER; //|DPNSESSION_MIGRATE_HOST;

    // We are now ready to host the app
    if (FAILED(hr = g_pDPServer->Host(&dpAppDesc,              // AppDesc
                                      &g_pDeviceAddress, 1,   // Device Address
                                      NULL, NULL,             // Reserved
                                      NULL,                   // Player Context
                                      0)))                       // dwFlags
    {
        MonoPrint("Failed Hosting:  0x%X\n", hr);
        goto LCleanup;
    }
    else
    {
        MonoPrint("Currently Hosting...\n");
    }


LCleanup:
    return hr;
}




//-----------------------------------------------------------------------------
// Name: ConnectToSession()
// Desc: Connects to a DirectPlay session
//-----------------------------------------------------------------------------
HRESULT ConnectToSession()
{
    HRESULT                     hr = E_FAIL;
    DPN_APPLICATION_DESC        dpnAppDesc;
    IDirectPlay8Address*        pHostAddress = NULL;


    ZeroMemory(&dpnAppDesc, sizeof(DPN_APPLICATION_DESC));
    dpnAppDesc.dwSize = sizeof(DPN_APPLICATION_DESC);
    dpnAppDesc.guidApplication = g_guidApp;

    // Simply connect to the first one in the list
    EnterCriticalSection(&g_csHostList);

    if (g_pHostList and SUCCEEDED(hr = g_pHostList->pHostAddress->Duplicate(&pHostAddress)))
    {
        hr = g_pDPClient->Connect(&dpnAppDesc,        // pdnAppDesc
                                  pHostAddress,       // pHostAddr
                                  g_pDeviceAddress,   // pDeviceInfo
                                  NULL,               // pdnSecurity
                                  NULL,               // pdnCredentials
                                  NULL, 0,            // pvUserConnectData/Size
                                  //  NULL,               // pvPlayerContext
                                  NULL,               // pvAsyncContext
                                  NULL,               // pvAsyncHandle
                                  DPNCONNECT_SYNC);   // dwFlags

        if (FAILED(hr))
            MonoPrint("Failed Connecting to Host:  0x%x\n", hr);
    }
    else
    {
        MonoPrint("Failed Duplicating Host Address:  0x%x\n", hr);
    }

    LeaveCriticalSection(&g_csHostList);

    SAFE_RELEASE(pHostAddress);
    return hr;
}




//-----------------------------------------------------------------------------
// Name: SendDirectPlayMessage()
// Desc: Sends a DirectPlay message to all players
//-----------------------------------------------------------------------------
HRESULT SendDirectPlayMessage()
{
    HRESULT         hr = S_OK;
    DPN_BUFFER_DESC dpnBuffer;
    WCHAR           wszData[256];

    // Get the data from the user
    MonoPrint("\nPlease Enter a String.\n");
    wscanf(L"%ls", wszData);

    dpnBuffer.pBufferData = (BYTE*) wszData;
    dpnBuffer.dwBufferSize = 2 * (wcslen(wszData) + 1);

    if (g_bHost)
    {
        if (FAILED(hr = g_pDPServer->SendTo(DPNID_ALL_PLAYERS_GROUP,   // dpnid
                                            &dpnBuffer,             // pBufferDesc
                                            1,                      // cBufferDesc
                                            0,                      // dwTimeOut
                                            NULL,                   // pvAsyncContext
                                            NULL,                   // pvAsyncHandle
                                            DPNSEND_SYNC |
                                            DPNSEND_NOLOOPBACK)))      // dwFlags
        {
            MonoPrint("Failed Sending Data:  0x%x\n", hr);
        }
    }
    else
    {
        if (FAILED(hr = g_pDPClient->Send( //DPNID_ALL_PLAYERS_GROUP,  // dpnid
                            &dpnBuffer,             // pBufferDesc
                            1,                      // cBufferDesc
                            0,                      // dwTimeOut
                            NULL,                   // pvAsyncContext
                            NULL,                   // pvAsyncHandle
                            DPNSEND_SYNC |
                            DPNSEND_NOLOOPBACK)))      // dwFlags
        {
            MonoPrint("Failed Sending Data:  0x%x\n", hr);
        }
    }

    return hr;
}




//-----------------------------------------------------------------------------
// Name: Register()
// Desc: Register app as lobby launchable
//-----------------------------------------------------------------------------
HRESULT Register()
{
    HRESULT             hr = S_OK;
    DPL_PROGRAM_DESC    dplDesc;
    WCHAR*              pwszPath = NULL;
    WCHAR*              pwszExecutable = NULL;
    int                 i;

    ZeroMemory(&dplDesc, sizeof(DPL_PROGRAM_DESC));
    dplDesc.dwSize = sizeof(DPL_PROGRAM_DESC);
    dplDesc.guidApplication = g_guidApp;

    // We need to parse out the path and the exe name from the input value
    for (i = wcslen(g_wszPath); i >= 0 and g_wszPath[i] not_eq L'\\'; i--);

    pwszPath = new WCHAR[i + 1];

    if (pwszPath == NULL)
    {
        MonoPrint("Failed parsing path.");
        hr = E_FAIL;
        goto LCleanup;
    }

    wcsncpy(pwszPath, g_wszPath, i);
    pwszPath[i] = L'\0';
    pwszExecutable = g_wszPath + i + 1;

    dplDesc.pwszApplicationName = pwszExecutable;
    dplDesc.pwszExecutableFilename = pwszExecutable;
    dplDesc.pwszExecutablePath = pwszPath;

    hr = g_pLobbyApp->RegisterProgram(&dplDesc, 0);
LCleanup:
    SAFE_DELETE_ARRAY(pwszPath);

    return hr;
}




//-----------------------------------------------------------------------------
// Name: UnRegister()
// Desc: Unregister app as lobby launchable
//-----------------------------------------------------------------------------
HRESULT UnRegister()
{
    HRESULT     hr = S_OK;

    hr = g_pLobbyApp->UnRegisterProgram(&g_guidApp, 0);

    return hr;
}




//-----------------------------------------------------------------------------
// Name: LobbyLaunch()
// Desc: Host or connect to session based on lobby launch settings
//-----------------------------------------------------------------------------
HRESULT LobbyLaunch()
{
    HRESULT                     hr = S_OK;
    return hr;
}




//-----------------------------------------------------------------------------
// Name: InitDirectPlayVoice()
// Desc: Initialize DirectPlay Voice
//-----------------------------------------------------------------------------
extern bool g_bServer;
HRESULT InitDirectPlayVoice()
{
    HRESULT             hr = S_OK;
    DVSESSIONDESC       dvSessionDesc;
    DVSOUNDDEVICECONFIG dvSoundDeviceConfig;
    DVCLIENTCONFIG      dvClientConfig;
    DVID                dvid;

    // Init the server if we are the host
    if (g_bHost)
    {
        // Create the IDirectPlayVoiceServer Object
        if (FAILED(hr = CoCreateInstance(CLSID_DirectPlayVoiceServer, NULL,
                                         CLSCTX_INPROC_SERVER,
                                         IID_IDirectPlayVoiceServer,
                                         (LPVOID*) &g_pVoiceServer)))
        {
            MonoPrint("Failed Creating the IDirectPlayVoiceServer Object:  0x%X\n", hr);
            goto LCleanup;
        }

        // Init the Voice server
        if (g_bHost)
        {
            if (FAILED(hr = g_pVoiceServer->Initialize(g_pDPServer, DirectVoiceServerMessageHandler, NULL, 0, 0)))
            {
                MonoPrint("Failed Initializing the IDirectPlayVoiceServer Object:  0x%X\n", hr);
                goto LCleanup;
            }
        }
        else
        {
            if (FAILED(hr = g_pVoiceServer->Initialize(g_pDPClient, DirectVoiceServerMessageHandler, NULL, 0, 0)))
            {
                MonoPrint("Failed Initializing the IDirectPlayVoiceServer Object:  0x%X\n", hr);
                goto LCleanup;
            }
        }

        ZeroMemory(&dvSessionDesc, sizeof(DVSESSIONDESC));
        dvSessionDesc.dwSize = sizeof(DVSESSIONDESC);

        if (g_bServer)
            dvSessionDesc.dwSessionType = DVSESSIONTYPE_MIXING;
        else
            dvSessionDesc.dwSessionType = DVSESSIONTYPE_FORWARDING;

        dvSessionDesc.dwBufferQuality = DVBUFFERQUALITY_DEFAULT;
        dvSessionDesc.guidCT = DPVCTGUID_DEFAULT;
        dvSessionDesc.dwBufferAggressiveness = DVBUFFERAGGRESSIVENESS_DEFAULT;


        /////// find and set the codec

        LPDIRECTPLAYVOICECLIENT pVoiceClient        = NULL;
        LPDVCOMPRESSIONINFO     pdvCompressionInfo  = NULL;
        LPGUID  pGuid         = NULL;
        LPBYTE  pBuffer       = NULL;
        DWORD   dwSize        = 0;
        DWORD   dwNumElements = 0;
        HRESULT hr;

        CoInitializeEx(NULL, COINIT_MULTITHREADED);

        if (FAILED(hr = CoCreateInstance(CLSID_DirectPlayVoiceClient, NULL,
                                         CLSCTX_INPROC_SERVER, IID_IDirectPlayVoiceClient,
                                         (VOID**) &pVoiceClient)))
        {
            return 1;
        }

        hr = pVoiceClient->GetCompressionTypes(pBuffer, &dwSize, &dwNumElements, 0);

        if (hr not_eq DVERR_BUFFERTOOSMALL and FAILED(hr))
        {
            return 1;
        }

        pBuffer = new BYTE[dwSize];

        if (FAILED(hr = pVoiceClient->GetCompressionTypes(pBuffer, &dwSize,
                        &dwNumElements, 0)))
        {
            return 1;
        }

        SAFE_RELEASE(pVoiceClient);
        CoUninitialize();

        pdvCompressionInfo = (LPDVCOMPRESSIONINFO) pBuffer;

        for (DWORD dwIndex = 0; dwIndex < dwNumElements; dwIndex++)
        {

            if (g_bLowBwVoice)
            {
                if (pdvCompressionInfo[dwIndex].guidType == DPVCTGUID_VR12)
                {
                    pGuid = new GUID;
                    (*pGuid) = pdvCompressionInfo[dwIndex].guidType;
                    break;
                }
            }
            else
            {
                if (pdvCompressionInfo[dwIndex].guidType == DPVCTGUID_SC03)
                {
                    pGuid = new GUID;
                    (*pGuid) = pdvCompressionInfo[dwIndex].guidType;
                    break;
                }
            }

        }

        SAFE_DELETE_ARRAY(pBuffer);

        dvSessionDesc.guidCT = (*pGuid);

        ///////////////////////////////////////////////////


        if (FAILED(hr = g_pVoiceServer->StartSession(&dvSessionDesc, 0)))
        {
            MonoPrint("Failed Starting the Session:  0x%X\n", hr);
            goto LCleanup;
        }
    }

    /// setup the client

    // Test DirectVoice
    if (FAILED(hr = TestDirectVoice()))
    {
        MonoPrint("Failed Testing DirectVoice:  0x%X\n", hr);
        goto LCleanup;
    }

    // Init the client and the connection
    // Create the IDirectPlayVoiceClient Object
    if (FAILED(hr = CoCreateInstance(CLSID_DirectPlayVoiceClient, NULL,
                                     CLSCTX_INPROC_SERVER,
                                     IID_IDirectPlayVoiceClient,
                                     (LPVOID*) &g_pVoiceClient)))
    {
        MonoPrint("Failed Creating the IDirectPlayVoiceClient Object:  0x%X\n", hr);
        goto LCleanup;
    }

    // Init the Voice client
    if (g_bHost)
    {
        if (FAILED(hr = g_pVoiceClient->Initialize(g_pDPServer, DirectVoiceClientMessageHandler, NULL, 0, 0)))
        {
            MonoPrint("Failed Initializing the IDirectPlayVoiceClient Object:  0x%X\n", hr);
            goto LCleanup;
        }
    }
    else
    {
        if (FAILED(hr = g_pVoiceClient->Initialize(g_pDPClient, DirectVoiceClientMessageHandler, NULL, 0, 0)))
        {
            MonoPrint("Failed Initializing the IDirectPlayVoiceClient Object:  0x%X\n", hr);
            goto LCleanup;
        }
    }

    ZeroMemory(&dvSoundDeviceConfig, sizeof(DVSOUNDDEVICECONFIG));
    dvSoundDeviceConfig.dwSize = sizeof(DVSOUNDDEVICECONFIG);
    dvSoundDeviceConfig.dwFlags =  DVSOUNDCONFIG_SETCONVERSIONQUALITY;
    dvSoundDeviceConfig.guidPlaybackDevice = DSDEVID_DefaultVoicePlayback;
    dvSoundDeviceConfig.lpdsPlaybackDevice = NULL;
    dvSoundDeviceConfig.guidCaptureDevice = DSDEVID_DefaultVoiceCapture;
    dvSoundDeviceConfig.lpdsCaptureDevice = NULL;
    dvSoundDeviceConfig.hwndAppWindow = GetConsoleHwnd();
    dvSoundDeviceConfig.lpdsMainBuffer = NULL;
    dvSoundDeviceConfig.dwMainBufferFlags = 0;
    dvSoundDeviceConfig.dwMainBufferPriority = 0;

    ZeroMemory(&dvClientConfig, sizeof(DVCLIENTCONFIG));
    dvClientConfig.dwSize = sizeof(DVCLIENTCONFIG);
    dvClientConfig.dwFlags = /*DVCLIENTCONFIG_AUTOVOICEACTIVATED bitor */DVCLIENTCONFIG_AUTORECORDVOLUME;
    dvClientConfig.lRecordVolume = DVRECORDVOLUME_LAST;
    dvClientConfig.lPlaybackVolume = DVPLAYBACKVOLUME_DEFAULT;
    dvClientConfig.dwThreshold = DVTHRESHOLD_UNUSED;
    dvClientConfig.dwBufferQuality = DVBUFFERQUALITY_MAX;
    dvClientConfig.dwBufferAggressiveness = DVBUFFERAGGRESSIVENESS_MAX;
    dvClientConfig.dwNotifyPeriod = 0;

    if (FAILED(hr = g_pVoiceClient->Connect(&dvSoundDeviceConfig, &dvClientConfig, DVFLAGS_SYNC)))
    {
        MonoPrint("Failed DirectVoice Client Connect:  0x%X\n", hr);
        goto LCleanup;
    }

    dvid = DVID_ALLPLAYERS;

    if (FAILED(hr = g_pVoiceClient->SetTransmitTargets(&dvid, 1, 0)))
    {
        MonoPrint("Failed SetTransmitTargets:  0x%X\n", hr);
        goto LCleanup;
    }

LCleanup:
    return hr;
}




//-----------------------------------------------------------------------------
// Name: TestDirectVoice()
// Desc: Tests that DirectPlay Voice is has been setup, and if it hasn't,
//       it runs the DirectPlay Voice setup wizard.
//-----------------------------------------------------------------------------
HRESULT TestDirectVoice()
{
    HRESULT                 hr = S_OK;
    IDirectPlayVoiceTest*   pVoiceTest = NULL;
    GUID                    guidPlayback;
    GUID                    guidCapture;

    // Create the IDirectPlayVoiceTest Object
    if (FAILED(hr = CoCreateInstance(CLSID_DirectPlayVoiceTest, NULL,
                                     CLSCTX_INPROC_SERVER,
                                     IID_IDirectPlayVoiceTest,
                                     (LPVOID*) &pVoiceTest)))
    {
        MonoPrint("Failed Creating the IDirectPlayVoiceTest Object:  0x%X\n", hr);
        goto LCleanup;
    }

    guidPlayback = DSDEVID_DefaultVoicePlayback;
    guidCapture = DSDEVID_DefaultVoiceCapture;

    hr = pVoiceTest->CheckAudioSetup(&guidPlayback, &guidCapture, NULL, DVFLAGS_QUERYONLY);

    if (hr == DVERR_RUNSETUP)
    {
        // this wizard is run through the falconinstaller
        /*  hwnd = NULL;//GetConsoleHwnd();

          if( FAILED( hr = pVoiceTest->CheckAudioSetup(&guidPlayback, &guidCapture, hwnd, DVFLAGS_ALLOWBACK ) ) )
          {
              MonoPrint("Failed CheckAudioSetup:  0x%X\n", hr);
              goto LCleanup;
          }*/
    }
    else if (FAILED(hr))
    {
        MonoPrint("Failed CheckAudioSetup:  0x%X\n", hr);
        goto LCleanup;
    }

LCleanup:
    SAFE_RELEASE(pVoiceTest);
    return hr;
}




//-----------------------------------------------------------------------------
// Name: GetConsoleHwnd()
// Desc: Returns the console HWND
//-----------------------------------------------------------------------------
extern HWND mainAppWnd;
HWND GetConsoleHwnd()
{
    /*  HWND hwndFound;        // This is what is returned to the caller.
      char pszNewWindowTitle[1024]; // Contains fabricated WindowTitle.
      char pszOldWindowTitle[1024]; // Contains original WindowTitle.

      // Fetch current window title.
      GetConsoleTitle(pszOldWindowTitle, 1024);

      // Format a "unique" NewWindowTitle.
      wsprintf(pszNewWindowTitle,"%d/%d", GetTickCount(), GetCurrentProcessId());

      // Change current window title.
      SetConsoleTitle(pszNewWindowTitle);

      // Ensure window title has been updated.
      Sleep(40);

      // Look for NewWindowTitle.
    char test[1024] = "FF 3D Output";
    //    hwndFound=FindWindow(NULL, pszNewWindowTitle);
    hwndFound=FindWindow(NULL, test);
      // Restore original window title.
      SetConsoleTitle(pszOldWindowTitle);

      return(hwndFound);*/
    return (mainAppWnd);
}

extern char* g_ipadress; // 2002-02-07 S.G.

//-----------------------------------------------------------------------------
// Name: CleanupDirectPlay()
// Desc: Cleanup DirectPlay
//-----------------------------------------------------------------------------
void CleanupDirectPlay()
{
    HOST_NODE*                  pHostNode = NULL;
    HOST_NODE*                  pHostNodetmp = NULL;

    // Shutdown DirectVoice
    if (g_pVoiceClient)
    {
        g_pVoiceClient->Disconnect(DVFLAGS_SYNC);
    }

    if (g_pVoiceServer)
    {
        g_pVoiceServer->StopSession(0);
    }

    // Shutdown DirectPlay
    if (g_bHost)
    {
        if (g_pDPServer)
            g_pDPServer->Close(0);
    }
    else if (g_ipadress)
    {
        if (g_pDPClient)
        {
            g_pDPClient->Close(0);
        }
    }


    if (g_pLobbyApp and g_ipadress)
        g_pLobbyApp->Close(0);

    // Clean up Host list
    if (g_pHostList)
    {
        EnterCriticalSection(&g_csHostList);

        pHostNode = g_pHostList;

        while (pHostNode not_eq NULL)
        {
            SAFE_RELEASE(pHostNode->pHostAddress);
            SAFE_DELETE(pHostNode->pAppDesc);
            SAFE_DELETE(pHostNode->pwszSessionName);

            pHostNodetmp = pHostNode;
            pHostNode    = pHostNode->pNext;
            SAFE_DELETE(pHostNodetmp);
        }

        LeaveCriticalSection(&g_csHostList);


        if (g_pDeviceAddress) SAFE_RELEASE(g_pDeviceAddress);

        if (g_pHostAddress)SAFE_RELEASE(g_pHostAddress);

        if (g_bHost and g_pDPServer)
        {
            SAFE_RELEASE(g_pDPServer);
        }
        else if (g_pDPClient)
        {
            SAFE_RELEASE(g_pDPClient);
        }

        if (g_pLobbyApp)SAFE_RELEASE(g_pLobbyApp);

        if (g_pVoiceServer)SAFE_RELEASE(g_pVoiceServer);

        if (g_pVoiceClient)SAFE_RELEASE(g_pVoiceClient);

        if (g_wszPath)SAFE_DELETE_ARRAY(g_wszPath);
    }

    if (g_bVoiceCom and not g_pDPServer and not g_pDPClient and (g_ipadress))  // 2002-02-07 ADDED BY S.G. ONLY IF CREATED AND WE'RE IN g_bVoiceCom MODE
        DeleteCriticalSection(&g_csHostList);
}

/*VOID DXUtil_ConvertWideStringToGeneric( TCHAR* tstrDestination, const WCHAR* wstrSource,
                                        int cchDestChar )
{
    if( tstrDestination==NULL or wstrSource==NULL )
        return;

#ifdef _UNICODE
    if( cchDestChar == -1 )
     wcscpy( tstrDestination, wstrSource );
    else
     wcsncpy( tstrDestination, wstrSource, cchDestChar );
#else
    DXUtil_ConvertWideStringToAnsi( tstrDestination, wstrSource, cchDestChar );
#endif
}
VOID DXUtil_ConvertWideStringToAnsi( CHAR* strDestination, const WCHAR* wstrSource,
                                     int cchDestChar )
{
    if( strDestination==NULL or wstrSource==NULL )
        return;

    if( cchDestChar == -1 )
        cchDestChar = wcslen(wstrSource)+1;

    WideCharToMultiByte( CP_ACP, 0, wstrSource, -1, strDestination,
                         cchDestChar-1, NULL, NULL );

    strDestination[cchDestChar-1] = 0;
}*/
//-----------------------------------------------------------------------------
// Name: main()
// Desc: Entry point for the application.
//-----------------------------------------------------------------------------
void startupvoice(char *ip)
{
    if (g_bconected) return;

    HRESULT                     hr;
    int                         iUserChoice;
    char hostIp[256];
    strcpy(hostIp, ip);

    if ( not strcmp(hostIp, "0.0.0.0")) g_bHost = TRUE;
    else g_bHost = FALSE;

    // Init COM so we can use CoCreateInstance
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    InitializeCriticalSection(&g_csHostList);

    // Init the DirectPlay system
    if (FAILED(hr = InitDirectPlay()))
    {
        MonoPrint("Failed Initializing DirectPlay:  0x%X\n", hr);
        goto LCleanup;
    }

    if (g_bRegister)
    {
        if (FAILED(hr = Register()))
        {
            MonoPrint("Failed To Register:  0x%X\n", hr);
        }

        goto LCleanup;
    }
    else if (g_bUnRegister)
    {
        if (FAILED(hr = UnRegister()))
        {
            MonoPrint("Failed To Unregister:  0x%X\n", hr);
        }

        goto LCleanup;
    }

    // See if we were lobby launched or not
    if (g_bLobbyLaunched)
    {
        if (FAILED(hr = LobbyLaunch()))
        {
            MonoPrint("Failed be Lobby Launched:  0x%X\n", hr);
            goto LCleanup;
        }
    }
    else
    {
        // Get the necessary user input on whether they are hosting or connecting

        if ( not strcmp(hostIp, "0.0.0.0")) iUserChoice = USER_HOST;
        else iUserChoice = USER_CONNECT;

        if (FAILED(hr = CreateDeviceAddress()))
        {
            MonoPrint("Failed CreatingDeviceAddress:  0x%X\n", hr);
            goto LCleanup;
        }

        if (iUserChoice == USER_HOST)
        {
            g_bHost = TRUE;

            if (FAILED(hr = HostSession()))
            {
                MonoPrint("Failed Hosting:  0x%X\n", hr);
                goto LCleanup;
            }
            else  g_bconected = true;
        }
        else
        {
            // set name
            if (g_pDPClient)
            {
                DPN_PLAYER_INFO pdpPlayerInfo   ;
                WCHAR namo[20];

                mbstowcs(&namo[0], LogBook.Name(), 20);

                ZeroMemory(&pdpPlayerInfo, sizeof(DPN_PLAYER_INFO));
                pdpPlayerInfo.pwszName = namo;
                pdpPlayerInfo.dwSize = sizeof(DPN_PLAYER_INFO);
                pdpPlayerInfo.dwInfoFlags = DPNINFO_NAME;
                DPNHANDLE hAsync;
                g_pDPClient->SetClientInfo(&pdpPlayerInfo, NULL, &hAsync, 0);
            }


            g_bHost = FALSE;

            if (FAILED(hr = EnumDirectPlayHosts(hostIp)))
            {
                MonoPrint("Failed Enumerating Host:  0x%X\n", hr);
                goto LCleanup;
            }

            if (FAILED(hr = ConnectToSession()))
            {
                MonoPrint("Failed Connect to Host:  0x%X\n", hr);
                goto LCleanup;
            }
            else
            {
                g_bconected = true;
                MonoPrint("\nConnection Successful.\n");
            }
        }
    }

    // Init DirectVoice
    if (FAILED(hr = InitDirectPlayVoice()))
    {
        MonoPrint("Failed Initializing DirectVoice:  0x%X\n", hr);
        goto LCleanup;
    }

    ////////////////////////
    return ;


    // no cleaning up for now ;(
LCleanup:
    CleanupDirectPlay();

    // ShutDown COM
    CoUninitialize();

    return ;
}

void RefreshVoiceFreqs()
{
    Package pkg;
    Flight flt;
    BOOL retval = FALSE;
    static int com1 = NULL;
    static int com2 = NULL;
    static int team = NULL;
    static bool init = false;
    static char* gamename;


    if ( not VM) return;

    if (g_bHost and not g_pDPServer)return;

    if ( not g_bHost and not g_pDPClient)return;

    if ( not g_bconected) return;

    if (g_bHost and not g_dpnidLocalPlayer) return;

    if ( not g_bHost and (g_afreqarrey.Freq[0][0] == 0 or g_afreqarrey.Freq[1][0] == 0 or g_afreqarrey.Freq[2][0] == 0)) return;

    if ( not g_afreqarrey.count and not init)
    {
        SetListenFreqsClient(11, 1234, 1);
        init = true;
    }

    if (team == FalconLocalSession->GetTeam() and com1 == VM->radiofilter[0] and com2 == VM->radiofilter[1]
       and FalconLocalSession->Game() and not strcmp(gamename, FalconLocalSession->Game()->GameName())) return;
    else if ( not FalconLocalSession->Game())return;
    else
    {
        // something on the radio changed...lets update the groups
        com1 = VM->radiofilter[0];
        com2 = VM->radiofilter[1];
        team = FalconLocalSession->GetTeam();
        gamename = FalconLocalSession->Game()->GameName();
    }

    VU_ID comvuid1;
    VU_ID comvuid2;


    switch (VM->radiofilter[0])
    {
        case rcfFlight1:
            comvuid1.num_ = FalconLocalSession->GetPlayerFlightID();
            break;

        case rcfFlight2:
            comvuid1.num_ = FalconLocalSession->GetPlayerFlightID() + 1;
            break;

        case rcfFlight3:
            comvuid1.num_ = FalconLocalSession->GetPlayerFlightID() + 2;
            break;

        case rcfFlight4:
            comvuid1.num_ = FalconLocalSession->GetPlayerFlightID() + 3;
            break;

        case rcfFlight5:
            comvuid1.num_ = FalconLocalSession->GetPlayerFlightID() + 4;
            break;

        case rcfPackage1:
        case rcfFromPackage:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid1.num_ = pkg->Id();
            break;

        case rcfPackage2:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid1.num_ = pkg->Id() + 1;
            break;

        case rcfPackage3:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid1.num_ = pkg->Id() + 2;
            break;


        case rcfPackage4:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid1.num_ = pkg->Id() + 3;
            break;


        case rcfPackage5:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid1.num_ = pkg->Id() + 4;
            break;

        case rcfProx: // 40nm range
            break;

        case rcfTeam:
            comvuid1.num_ = FalconLocalSession->GetTeam() + 10;
            break;

        case rcfAll:
            comvuid1.num_ = 1;//guard
            break;

        case rcfTower:
            if (gNavigationSys and SimDriver.GetPlayerEntity())
            {
                VU_ID ATCId;
                gNavigationSys->GetAirbase(&ATCId);
                comvuid1.num_ = ATCId;
            }

            break;
    }

    switch (VM->radiofilter[1])
    {
        case rcfFlight1:
            comvuid2.num_ = FalconLocalSession->GetPlayerFlightID();
            break;

        case rcfFlight2:
            comvuid2.num_ = FalconLocalSession->GetPlayerFlightID() + 1;
            break;

        case rcfFlight3:
            comvuid2.num_ = FalconLocalSession->GetPlayerFlightID() + 2;
            break;

        case rcfFlight4:
            comvuid2.num_ = FalconLocalSession->GetPlayerFlightID() + 3;
            break;

        case rcfFlight5:
            comvuid2.num_ = FalconLocalSession->GetPlayerFlightID() + 4;
            break;

        case rcfPackage1:
        case rcfFromPackage:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid2.num_ = pkg->Id();
            break;

        case rcfPackage2:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid2.num_ = pkg->Id() + 1;
            break;

        case rcfPackage3:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid2.num_ = pkg->Id() + 2;
            break;

        case rcfPackage4:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid2.num_ = pkg->Id() + 3;
            break;

        case rcfPackage5:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid2.num_ = pkg->Id() + 4;
            break;

        case rcfProx: // 40nm range
            break;

        case rcfTeam:
            comvuid2.num_ = FalconLocalSession->GetTeam() + 10;
            break;

        case rcfAll:
            comvuid2.num_ = 1;//guard
            break;

        case rcfTower:
            if (gNavigationSys and SimDriver.GetPlayerEntity())
            {
                VU_ID ATCId;
                gNavigationSys->GetAirbase(&ATCId);
                comvuid2.num_ = ATCId;
            }

            break;
    }

    unsigned long guard = 1;

    if (FalconLocalSession->GetFlyState() == FLYSTATE_IN_UI)
    {
        if (FalconLocalSession->GetTeam() not_eq 255)
            comvuid1.num_ = FalconLocalSession->GetTeam() + 10; //ui
        else comvuid1.num_ = 11;//ui

        comvuid2.num_ = 1234;//ui
    }

    SetListenFreqsClient(comvuid1.num_, comvuid2.num_, guard);

}

void Transmit(int com)
{

    Package pkg;
    Flight flt;
    BOOL retval = FALSE;
    static int com1 = NULL;
    static int com2 = NULL;
    bool doupdate = false;

    if ( not VM) return;

    if (g_bHost and not g_pDPServer) return;

    if ( not g_bHost and not g_pDPClient) return;

    if (g_bHost and not g_dpnidLocalPlayer) return;


    VU_ID comvuid1;
    VU_ID comvuid2;

    switch (VM->radiofilter[0])
    {
        case rcfFlight1:
            comvuid1.num_ = FalconLocalSession->GetPlayerFlightID();
            break;

        case rcfFlight2:
            comvuid1.num_ = FalconLocalSession->GetPlayerFlightID() + 1;
            break;

        case rcfFlight3:
            comvuid1.num_ = FalconLocalSession->GetPlayerFlightID() + 2;
            break;

        case rcfFlight4:
            comvuid1.num_ = FalconLocalSession->GetPlayerFlightID() + 3;
            break;

        case rcfFlight5:
            comvuid1.num_ = FalconLocalSession->GetPlayerFlightID() + 4;
            break;

        case rcfPackage1:
        case rcfFromPackage:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid1 = pkg->Id();
            break;

        case rcfPackage2:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid1.num_ = pkg->Id() + 1;
            break;

        case rcfPackage3:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid1.num_ = pkg->Id() + 2;
            break;

        case rcfPackage4:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid1.num_ = pkg->Id() + 3;
            break;

        case rcfPackage5:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid1.num_ = pkg->Id() + 4;
            break;

        case rcfProx: // 40nm range
            break;

        case rcfTeam:
            comvuid1.num_ = FalconLocalSession->GetTeam() + 10;
            break;

        case rcfAll:
            comvuid1.num_ = 1;//guard
            break;

        case rcfTower:
            if (gNavigationSys and SimDriver.GetPlayerEntity())
            {
                VU_ID ATCId;
                gNavigationSys->GetAirbase(&ATCId);
                comvuid1.num_ = ATCId;
            }

            break;
    }

    switch (VM->radiofilter[1])
    {
        case rcfFlight1:
            comvuid2.num_ = FalconLocalSession->GetPlayerFlightID() ;
            break;

        case rcfFlight2:
            comvuid2.num_ = FalconLocalSession->GetPlayerFlightID() + 1;
            break;

        case rcfFlight3:
            comvuid2.num_ = FalconLocalSession->GetPlayerFlightID() + 2;
            break;

        case rcfFlight4:
            comvuid2.num_ = FalconLocalSession->GetPlayerFlightID() + 3;
            break;

        case rcfFlight5:
            comvuid2.num_ = FalconLocalSession->GetPlayerFlightID() + 4;
            break;

        case rcfPackage1:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid2 = pkg->Id();
            break;

        case rcfPackage2:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid2.num_ = pkg->Id() + 1;
            break;

        case rcfPackage3:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid2.num_ = pkg->Id() + 2;
            break;

        case rcfPackage4:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid2.num_ = pkg->Id() + 3;
            break;

        case rcfPackage5:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid2.num_ = pkg->Id() + 4;
            break;

        case rcfFromPackage:
            flt = (Flight)vuDatabase->Find(FalconLocalSession->GetPlayerFlightID());

            if ( not flt)
                break;

            pkg = (Package)flt->GetUnitParent();

            if ( not pkg)
                break;

            comvuid2 = pkg->Id();
            break;

        case rcfProx: // 40nm range
            break;

        case rcfTeam:
            comvuid2.num_ = FalconLocalSession->GetTeam() + 10;
            break;

        case rcfAll:
            comvuid2.num_ = 1;//guard
            break;

        case rcfTower:
            if (gNavigationSys and SimDriver.GetPlayerEntity())
            {
                VU_ID ATCId;
                gNavigationSys->GetAirbase(&ATCId);
                comvuid2.num_ = ATCId;
            }

            break;
    }

    if (FalconLocalSession->GetFlyState() == FLYSTATE_IN_UI)
    {
        if (FalconLocalSession->GetTeam() not_eq 255)
            comvuid1.num_ = FalconLocalSession->GetTeam() + 10; //ui
        else comvuid1.num_ = 11;//ui

        comvuid2.num_ = 1234;//ui

    }

    if (com == 0)
    {
        //DVID dvid = NULL;
        if ( not g_pVoiceClient or FAILED(g_pVoiceClient->SetTransmitTargets(0, 0, 0)))
        {
            MonoPrint("Failed SetTransmitTargets:  0x%X\n");
        }
    }
    else if (com == 1)
    {
        TransmistoFreq(comvuid1.num_);
        g_itransmitfreq = 1;
    }
    else if (com == 2)
    {
        TransmistoFreq(comvuid2.num_);
        g_itransmitfreq = 2;
    }
}


void TransmistoFreq(unsigned long transmitfreq)
{
    bool found = false;

    // find the Freq
    for (int i = 0; i < g_afreqarrey.count; i++)
    {
        if (transmitfreq == g_afreqarrey.Freq[i][1]) // corect game
        {
            // Talk to just this group
            g_pVoiceClient->SetTransmitTargets(&g_afreqarrey.Freq[i][0], 1, 0);
            found = true;
        }

    }

    if ( not found) SetListenFreqsClient(11, 1234, 1);
}

void SetListenFreqsClient(unsigned long com1, unsigned long com2, unsigned long guard)
{
    if (g_bHost)
    {
        SetListenFreqsHost(g_dpnidLocalPlayer, com1, com2, guard);
        return;
    }

    HRESULT         hr = S_OK;
    DPN_BUFFER_DESC dpnBuffer;
    COM_MESSAGE_SetClientListenFreqs  comData;
    comData.dwType = GAME_MSGID_SetClientListenFreqs;
    comData.dpnid = g_dpnidLocalPlayer;
    comData.com[0] = com1;
    comData.com[1] = com2;
    comData.com[2] = guard;
    strcpy(comData.gamename , vuLocalSessionEntity->Game()->GameName());
    dpnBuffer.pBufferData = (BYTE*) &comData;
    dpnBuffer.dwBufferSize = sizeof(COM_MESSAGE_SetClientListenFreqs);

    if (FAILED(hr = g_pDPClient->Send(&dpnBuffer,              // pBufferDesc
                                      1,                      // cBufferDesc
                                      0,                      // dwTimeOut
                                      NULL,                   // pvAsyncContext
                                      NULL,                   // pvAsyncHandle
                                      DPNSEND_SYNC |
                                      DPNSEND_GUARANTEED |
                                      DPNSEND_NOLOOPBACK)))     // dwFlags
    {
        printf("Failed Sending Data:  0x%x\n", hr);
    }

    return ;


}

void SetListenFreqsHost(DPNID playerid, unsigned long com1, unsigned long com2, unsigned long guard)
{
    //first we find the current freqs on the server
    HRESULT hr;
    // Enumerate all groups to figure out which group we are target, and joining

    bool com1ok = false;
    bool com2ok = false;
    bool guardok = false;

start:
    DPNID* aGroupsDPNID = NULL;
    DWORD dwCount = 0;

    do
    {
        SAFE_DELETE_ARRAY(aGroupsDPNID);

        if (dwCount)
            aGroupsDPNID = new DPNID[ dwCount ];

        hr = g_pDPServer->EnumPlayersAndGroups(aGroupsDPNID, &dwCount, DPNENUM_GROUPS);
    }
    while (hr == DPNERR_BUFFERTOOSMALL and dwCount not_eq 0);

    // find the Freq if it's already there and join it

    for (DWORD i = 0; i < dwCount; i++)
    {
        ////first we need to create the groups that are not already there
        DWORD dwGroupNumber;

        // Get group info
        DWORD dwSize = 0;
        DPN_GROUP_INFO* pdpGroupInfo = NULL;
        hr = g_pDPServer->GetGroupInfo(aGroupsDPNID[i], pdpGroupInfo, &dwSize, 0);

        if (FAILED(hr) and hr not_eq DPNERR_BUFFERTOOSMALL)
            return ;

        pdpGroupInfo = (DPN_GROUP_INFO*) new BYTE[ dwSize ];
        ZeroMemory(pdpGroupInfo, dwSize);
        pdpGroupInfo->dwSize = sizeof(DPN_GROUP_INFO);
        hr = g_pDPServer->GetGroupInfo(aGroupsDPNID[i], pdpGroupInfo, &dwSize, 0);

        if (FAILED(hr))
            return ;

        if (pdpGroupInfo->pvData)
            dwGroupNumber = *((DWORD*) pdpGroupInfo->pvData);
        else
            dwGroupNumber = 0;

        ////convert the name to the vuid
        TCHAR freq[sizeof(long) * 8 + 1] ;
        DXUtil_ConvertWideStringToGeneric(freq, pdpGroupInfo->pwszName, sizeof(long) * 8 + 1);
        char *p;
        const char* str = freq;
        unsigned long FreqVUID = strtoul(&str[0], &p, sizeof(long) * 8 + 1);

        //////////////////////////////
        if (com1 == FreqVUID) com1ok = TRUE;

        if (com2 == FreqVUID) com2ok = TRUE;

        if (guard == FreqVUID) guardok = TRUE;
    }

    if ( not com1ok and com1)
    {
        CreateGroup(com1);
        com1ok = true;
        goto start;
    }
    else if ( not com2ok and com2)
    {
        CreateGroup(com2);
        com2ok = true;
        goto start;
    }
    else if ( not guardok)
    {
        CreateGroup(guard);
        guardok = true;
        goto start;
    }


    // now all groups should exsist
    // lets join the requested groups and leave the rest
    for (unsigned i = 0; i < dwCount; i++)
    {
        DWORD dwGroupNumber;

        // Get group info
        DWORD dwSize = 0;
        DPN_GROUP_INFO* pdpGroupInfo = NULL;
        hr = g_pDPServer->GetGroupInfo(aGroupsDPNID[i], pdpGroupInfo, &dwSize, 0);

        if (FAILED(hr) and hr not_eq DPNERR_BUFFERTOOSMALL)
            return ;

        pdpGroupInfo = (DPN_GROUP_INFO*) new BYTE[ dwSize ];
        ZeroMemory(pdpGroupInfo, dwSize);
        pdpGroupInfo->dwSize = sizeof(DPN_GROUP_INFO);
        hr = g_pDPServer->GetGroupInfo(aGroupsDPNID[i], pdpGroupInfo, &dwSize, 0);

        if (FAILED(hr))
            return ;

        if (pdpGroupInfo->pvData)
            dwGroupNumber = *((DWORD*) pdpGroupInfo->pvData);
        else
            dwGroupNumber = 0;

        ////convert the name to the vuid
        TCHAR freq[sizeof(long) * 8 + 1] ;
        DXUtil_ConvertWideStringToGeneric(freq, pdpGroupInfo->pwszName, sizeof(long) * 8 + 1);
        char *p;
        const char* str = freq;
        unsigned long FreqVUID = strtoul(&str[0], &p, sizeof(long) * 8 + 1);

        //////////////////////////////
        if (com1 == FreqVUID or com2 == FreqVUID or guard == FreqVUID)
        {
            // Add player to this group
            DPNHANDLE hAsync;
            hr = g_pDPServer->AddPlayerToGroup(aGroupsDPNID[i], playerid,
                                               NULL, &hAsync, 0);
        }
        else
        {
            //remove player from group
            DPNHANDLE hAsync;
            hr = g_pDPServer->RemovePlayerFromGroup(aGroupsDPNID[i],
                                                    playerid,
                                                    NULL, &hAsync, 0);
        }

        SAFE_DELETE_ARRAY(pdpGroupInfo);

        if (dwGroupNumber == 0)
            continue;
    }

}



void CreateGroup(unsigned long freq)
{
    HRESULT     hr = S_OK;
    DPNID* aGroupsDPNID = NULL;
    DWORD dwCount = 0;

    do
    {
        SAFE_DELETE_ARRAY(aGroupsDPNID);

        if (dwCount)
            aGroupsDPNID = new DPNID[ dwCount ];

        hr = g_pDPServer->EnumPlayersAndGroups(aGroupsDPNID, &dwCount, DPNENUM_GROUPS);
    }
    while (hr == DPNERR_BUFFERTOOSMALL and dwCount not_eq 0);

    DPN_GROUP_INFO dpGroupInfo;
    DWORD* pdwData = new DWORD;
    *pdwData = dwCount + 1;
    ZeroMemory(&dpGroupInfo, sizeof(DPN_GROUP_INFO));
    dpGroupInfo.dwSize = sizeof(DPN_GROUP_INFO);
    dpGroupInfo.dwInfoFlags = DPNINFO_NAME bitor DPNINFO_DATA ;
    dpGroupInfo.dwGroupFlags = DPNGROUP_AUTODESTRUCT;
    dpGroupInfo.pvData = pdwData;
    dpGroupInfo.dwDataSize = sizeof(DWORD);
    TCHAR buffer [sizeof(long) * 8 + 1];
    ltoa(freq, buffer, (sizeof(long) * 8 + 1));
    WCHAR name[sizeof(long) * 8 + 1];
    mbstowcs(&name[0], buffer, sizeof(long) * 8 + 1);
    dpGroupInfo.pwszName = name;
    DPNHANDLE hAsync;
    hr = g_pDPServer->CreateGroup(&dpGroupInfo, NULL,
                                  NULL, &hAsync, 0);
}

void SendFreqid(DPNID dpnidplayer, DPNID dpnidgroup, unsigned long freq)
{
    HRESULT         hr = S_OK;
    DPN_BUFFER_DESC dpnBuffer;
    COM_MESSAGE_SetFreqId  comData;
    DWORD flags;


    // lets find the freqs
    comData.dwType =  GAME_MSGID_SetFreqId;
    comData.Freq[0] = dpnidgroup;
    comData.Freq[1] = freq;

    dpnBuffer.pBufferData = (BYTE*) &comData;
    dpnBuffer.dwBufferSize = sizeof(COM_MESSAGE_SetFreqId);

    if ( not dpnidplayer and g_bHost)
    {
        dpnidplayer = DPNID_ALL_PLAYERS_GROUP;
        flags = DPNSEND_SYNC bitor DPNSEND_GUARANTEED;
    }
    else flags = DPNSEND_SYNC bitor DPNSEND_GUARANTEED bitor DPNSEND_NOLOOPBACK;

    if (FAILED(hr = g_pDPServer->SendTo(dpnidplayer,    // dpnid
                                        &dpnBuffer,             // pBufferDesc
                                        1,                      // cBufferDesc
                                        0,                      // dwTimeOut
                                        NULL,                   // pvAsyncContext
                                        NULL,                   // pvAsyncHandle
                                        flags)))      // dwFlags
    {
        printf("Failed Sending Data:  0x%x\n", hr);
    }

    return ;
}
