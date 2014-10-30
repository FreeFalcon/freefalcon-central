#include <objbase.h>
#include <cguid.h>
#include <dplay8.h>
#include <initguid.h>

#include "F4Comms.h"
#include "falclib.h"
#include "capiopt.h"
#include "Comms/udp.h"
#include "Comms/rudp.h"
#include "router.h" //KCK This needs to go away
#include "UI/INCLUDE/uicomms.h" // UI comms manager
#include "falclib/include/msginc/sendchatmessage.h"
#include "FALCLIB/INCLUDE/f4find.h"
#include "FalcMesg.h"
#include "MsgInc/TimingMsg.h"
#include "Falcmesg.h"
#include "DispCfg.h"
#include "CmpClass.h"
#include "ComData.h"
#include "UI/INCLUDE/queue.h"
#include "UI/INCLUDE/falcuser.h"
#include "TimerThread.h"
#include "acselect.h"
#include "pilot.h"
#include "flight.h"
#include "voicecomunication/voicecom.h"
#include "FALCLIB/INCLUDE/MsgInc/PlayerStatusMsg.h"
#include "FALCLIB/INCLUDE/MsgInc/SimCampMsg.h"
#include "aircrft.h"
#include "SimBase.h"


// ==============================================
// Insert DPLAY crap here
// ==============================================

DEFINE_GUID(OVERRIDE_GUID, 0x126e6180, 0xd307, 0x11d0, 0x9c, 0x4f, 0x0, 0xa0, 0xc9, 0x5, 0x42, 0x5e);
GUID gOurGUID = OVERRIDE_GUID;

// ========================================================================
// Some defines
// ========================================================================

#define F4COMMS_CONNECTION_TIMEOUT_SECS 8

#define F4COMMS_CONNECTED_TO_NO_ONE 0
#define F4COMMS_PARTIALLY_CONNECTED 1
#define F4COMMS_FULLY_CONNECTED 2

// KCK: These really need to come from comms itself
#define COMMS_TCP_OVERHEAD 40
#define COMMS_UDP_OVERHEAD 12

#define F4COMMS_MAX_PACKETS 32

// ====================================
// Globals
// ====================================

com_API_handle FalconTCPListenHandle = NULL;
com_API_handle FalconGlobalUDPHandle = NULL;
com_API_handle FalconGlobalTCPHandle = NULL;
//com_API_handle FalconInitialUDPHandle = NULL;
int FalconServerTCPStatus = VU_CONN_INACTIVE;
int FalconConnectionProtocol = 0;
int FalconConnectionType = 0;
int FalconConnectionDescription = 0;
int gConnectionStatus = 0;
int gTimeModeServer = 0;
int g_b_forcebandwidth = 0;
extern bool g_bServer;


//DCNode *DanglingConnections = NULL;
FalconPrivateList *DanglingSessionsList = NULL;

// debug bandwidth limiters
int F4CommsBandwidth = 0;
int F4CommsLatency = 100;
int F4CommsDropInterval = 0;
int F4CommsSwapInterval = 0;
int F4SessionUpdateTime = 0;
int F4SessionAliveTimeout = 0;
int F4CommsMTU = 0; // MTU variable declared Unz
extern bool g_bF4CommsMTU; // Unz switch for MTU
// ========================================================================
// Message sizing variables
// ========================================================================

// Ideal packet size we'll send over the wire;
int F4CommsIdealPacketSize;
// Corrisponding content sizes
int F4CommsIdealTCPPacketSize;
int F4CommsIdealUDPPacketSize;
// Corrisponding messages sizes
int F4CommsMaxTCPMessageSize;
int F4CommsMaxUDPMessageSize;
// Maximum sized message vu can accept
int F4VuMaxTCPMessageSize;
int F4VuMaxUDPMessageSize;
// Maximum sized packet vu with pack messages into
int F4VuMaxTCPPackSize;
int F4VuMaxUDPPackSize;

// ====================================
// Prototypes: sfr: moved to header file
// ====================================

//void SendConnectionList (FalconSessionEntity *session);

// ====================================
// COMMS startup and shutdown stuff
// ====================================

// This sets up a comms session (both udp and tcp handles) using the current protocol.
// FalconGlobalHandles are handles to the whole world (ie: broadcast on a LAN, or a server address,
// or NULL if not available)
// tcpListenHandle is a listen handle where we can recieve tcp connection requests.
//
// InitCommsStuff takes the following parameters:
// int GameType: Nonzero if a game is currently running and needs to
// Attach to a group immediately.
// FalconConnectionType fct: FalconConnectionType enumerated type
// ulong IPAddress: IPAddress of server or other machine, if any.
//
// Returns < 0 on error.
char* g_ipadress = NULL;
extern IDirectPlay8Server* g_pDPServer;
extern IDirectPlay8Client* g_pDPClient;
extern bool g_bVoiceCom;
extern char g_strVoiceHostIP[0x40];
extern bool stoppingvoice;
extern bool g_bACPlayerCTDFix;

int InitCommsStuff(ComDataClass *comData)
{
    g_ipadress = ComAPIinet_htoa(comData->ip_address);//me123

    // we need to create both handles on startup, so as to haev the ports available
    com_API_handle tmpHandle = NULL, tmpHandle2 = NULL;
    int bigPipeBandwidth = -1;
    int smallPipeBandwidth = 2000;
    FalconTCPListenHandle = NULL;
    FalconGlobalUDPHandle = NULL;
    FalconGlobalTCPHandle = NULL;
    // FalconInitialUDPHandle = NULL;
    FalconServerTCPStatus = VU_CONN_INACTIVE;
    // DanglingConnections = NULL;

    // Shutdown any current games/connections
    SendMessage(FalconDisplay.appWin, FM_SHUTDOWN_CAMPAIGN, 0, 0);

    if (gConnectionStatus)
    {
        EndCommsStuff();
    }

    gConnectionStatus = F4COMMS_ERROR_UDP_NOT_AVAILABLE;
    FalconConnectionDescription = FCT_WAN;
    SetupMessageSizes(FCT_WAN);
    // start BW FSM and set our ports
    ComAPIBWStart();
    com_API_set_local_ports(comData->localPort, comData->localPort + 1);
    vuLocalSessionEntity->SetAddress(VU_ADDRESS(0, com_API_get_my_receive_port(), com_API_get_my_reliable_receive_port()));

    // group handles
    // UDP
    FalconGlobalUDPHandle = ComAPICreateGroup("CreateGroup WAN FalconGlobalUDPHandle\n", F4CommsMaxUDPMessageSize, 0);

    if ( not FalconGlobalUDPHandle)
    {
        return F4CommsConnectionCallback(F4COMMS_ERROR_UDP_NOT_AVAILABLE);
    }

    // TCP
    FalconGlobalTCPHandle = ComAPICreateGroup("WAN RUDP GROUP", F4CommsMaxTCPMessageSize, 0);

    if ( not FalconGlobalTCPHandle)
    {
        return F4CommsConnectionCallback(F4COMMS_ERROR_MULTICAST_NOT_AVAILABLE);
    }

    // server
    if (comData->ip_address == 0)
    {
        // this dangling represents the first client to connect to server
        // well receive initial data through it
        bool ret = AddDanglingSession(
                       VU_ID(CAPI_DANGLING_ID, VU_SESSION_ENTITY_ID),
                       VU_ADDRESS(CAPI_DANGLING_IP, com_API_get_my_receive_port(), com_API_get_my_reliable_receive_port())
                   );

        if ( not (ret))
        {
            return F4CommsConnectionCallback(F4COMMS_ERROR_UDP_NOT_AVAILABLE);
        }
    }
    // client
    else
    {
        // this is a dangling session representing server, so we can send first message
        bool ret = AddDanglingSession(
                       VU_ID(CAPI_DANGLING_ID, VU_SESSION_ENTITY_ID),
                       VU_ADDRESS(comData->ip_address, comData->remotePort, comData->remotePort + 1)
                   );

        if ( not (ret))
        {
            return F4CommsConnectionCallback(F4COMMS_ERROR_UDP_NOT_AVAILABLE);
        }
    }

    // We have UDP bitand RUDP and PTOP
    FalconConnectionProtocol = FCP_UDP_AVAILABLE bitor FCP_RUDP_AVAILABLE;
    FalconConnectionType = FCT_PTOP_AVAILABLE;
    F4CommsConnectionCallback(F4COMMS_CONNECTED);


    ComAPISetTimeStampFunction(TimeStampFunction);
    return gConnectionStatus;
}



// This gets called after we've made a connection (or failed for sure)
int F4CommsConnectionCallback(int result)
{
    if (result == F4COMMS_CONNECTED)
    {
        int vures;

        // Init vu's comms
        if ( not FalconGlobalTCPHandle)
        {
            vures = gMainThread->InitComms(
                        FalconGlobalUDPHandle, F4CommsMaxTCPMessageSize, F4CommsIdealTCPPacketSize,
                        FalconGlobalUDPHandle, F4CommsMaxTCPMessageSize, F4CommsIdealTCPPacketSize,
                        F4_EVENT_QUEUE_SIZE
                    );
        }
        else
        {
            vures = gMainThread->InitComms(
                        FalconGlobalUDPHandle, F4CommsMaxUDPMessageSize, F4CommsIdealUDPPacketSize,
                        FalconGlobalTCPHandle, F4CommsMaxTCPMessageSize, F4CommsIdealTCPPacketSize,
                        F4_EVENT_QUEUE_SIZE
                    );
        }

        if (vures == VU_ERROR)
        {
            // KCK: Vu often doesn't clean up after itself in this case
            CleanupComms();
            return F4COMMS_ERROR_UDP_NOT_AVAILABLE;
        }

        gConnectionStatus = F4COMMS_CONNECTED;
    }
    else
    {
        CleanupComms();
    }

    // KCK: Call UI Comms connect callback
    gCommsMgr->StartCommsDoneCB(result);
    return result;
}

int CleanupComms(void)
{
    gConnectionStatus = 0;

    if (FalconGlobalUDPHandle)
        ComAPIClose(FalconGlobalUDPHandle);

    if (FalconGlobalTCPHandle)
        ComAPIClose(FalconGlobalTCPHandle);

    if (FalconTCPListenHandle)
        ComAPIClose(FalconTCPListenHandle);

    // if (FalconInitialUDPHandle)
    // ComAPIClose(FalconInitialUDPHandle);

    // Kill off any dangling sessions
    CleanupDanglingList();

    // FalconInitialUDPHandle = NULL;
    FalconTCPListenHandle = NULL;
    FalconGlobalUDPHandle = NULL;
    FalconGlobalTCPHandle = NULL;
    FalconServerTCPStatus = VU_CONN_INACTIVE;
    return (TRUE);
}

int EndCommsStuff(void)
{
    if (gMainThread)
    {
        gMainThread->LeaveGame();
    }

    // KCK HACK: To avoid vu's problem with shutting down comms when remote sessions are active
    VuSessionsIterator siter(vuGlobalGroup);
    FalconSessionEntity *cs;

    for (cs = (FalconSessionEntity*) siter.GetFirst(); cs not_eq NULL;)
    {
        FalconSessionEntity *oldCs = cs;
        cs = (FalconSessionEntity*) siter.GetNext();
        oldCs->JoinGame(NULL);
    }

    // END HACK

    if (gConnectionStatus == F4COMMS_CONNECTED)
    {
        gMainThread->DeinitComms();
    }

    CleanupComms();

    return (TRUE);
}

void SetupMessageSizes(int protocol)
{
    // Ideal packet size comms will send over the wire
    if (g_bF4CommsMTU)
    {
        F4CommsIdealPacketSize = F4CommsMTU;  // Unz and Booster MTU tweek
    }
    else
    {
        F4CommsIdealPacketSize = 500;
    }

    // Corrisponding content sizes
    F4CommsIdealTCPPacketSize = F4CommsIdealPacketSize - COMMS_TCP_OVERHEAD;
    F4CommsIdealUDPPacketSize = F4CommsIdealPacketSize - COMMS_UDP_OVERHEAD;
    // Corrisponding messages sizes
    F4CommsMaxTCPMessageSize = F4CommsIdealTCPPacketSize * F4COMMS_MAX_PACKETS;
    F4CommsMaxUDPMessageSize = F4CommsIdealUDPPacketSize;
    // Maximum sized message vu can accept
    F4VuMaxTCPMessageSize = F4CommsMaxTCPMessageSize - PACKET_HDR_SIZE - MAX_MSG_HDR_SIZE;
    F4VuMaxUDPMessageSize = F4CommsMaxUDPMessageSize - PACKET_HDR_SIZE - MAX_MSG_HDR_SIZE;
    // Maximum sized packet vu with pack messages into
    F4VuMaxTCPPackSize = F4CommsIdealTCPPacketSize - PACKET_HDR_SIZE - MAX_MSG_HDR_SIZE;
    F4VuMaxUDPPackSize = F4CommsIdealUDPPacketSize - PACKET_HDR_SIZE - MAX_MSG_HDR_SIZE;
    protocol;
}

// =====================================
// Dangling session stuff
// =====================================

void InitDanglingList(void)
{
    // VuEnterCriticalSection();
    VuSessionFilter DanglingSessionFilter(FalconNullId);
    DanglingSessionsList = new FalconPrivateList(&DanglingSessionFilter);
    // VuExitCriticalSection();
}

void CleanupDanglingList(void)
{
    // VuEnterCriticalSection();
    if (DanglingSessionsList)
    {
        delete DanglingSessionsList;
    }

    DanglingSessionsList = NULL;
    // VuExitCriticalSection();
}

//sfr: converts
// changed prototype, changing ip for address and adding VU_ID
//void AddDanglingSession (com_API_handle ch1, com_API_handle ch2, VU_SESSION_ID id, VU_ADDRESS address)
bool AddDanglingSession(VU_ID owner, VU_ADDRESS address)
{
    VuEnterCriticalSection();
    FalconSessionEntity *tempSess = NULL;

    // first time in dangling session, why not use a default constructor
    if ( not DanglingSessionsList)
    {
        InitDanglingList();
    }

    // Check if it's already in our list
    {
        VuListIterator dsit(DanglingSessionsList);

        for (
            FalconSessionEntity *session = (FalconSessionEntity*)dsit.GetFirst();
            session not_eq NULL;
            session = (FalconSessionEntity*)dsit.GetNext()
        )
        {
            VU_ADDRESS sAdd = session->GetAddress();

            if (
                //(session->OwnerId().creator_.value_ == CAPI_DANGLING_ID) or
                (session->OwnerId().creator_.value_ == owner.creator_) and 
                (sAdd == address)
            )
            {
                // found one
                VuExitCriticalSection();
                return true;
                //tempSess = session;
                //break;
            }
        }
    }

    // session is not in list, create a new one and insert in list
    tempSess = new FalconSessionEntity(CAPI_DANGLING_ID, "tmp");
    tempSess->SetOwnerId(owner);
    tempSess->SetAddress(address);
    // KCK: Hackish. Need to trick VU into this is already inserted into DB, otherwise
    // it won't insert it into our list.
    tempSess->SetVuStateAccess(VU_MEM_ACTIVE);

    // temp session handle
    com_API_handle udpHandle = ComUDPOpen(
                                 "Dangling UDP",
                                 F4CommsMaxUDPMessageSize,
                                 vuxWorldName,
                                 vuLocalSessionEntity->GetAddress().recvPort,
                                 address.recvPort,
                                 address.ip,
                                 owner.creator_.value_
                             );

    if (udpHandle == NULL)
    {
        delete tempSess;
        VuExitCriticalSection();
        return false;
    }

    // handlers for this session
    tempSess->SetCommsHandle(udpHandle, F4CommsMaxUDPMessageSize, F4CommsIdealUDPPacketSize);
    tempSess->SetCommsStatus(VU_CONN_ACTIVE);

    // add handlers to UDP group handler (so we can send and receive data from it)
    ComAPIAddToGroup(FalconGlobalUDPHandle, udpHandle);

    DanglingSessionsList->ForcedInsert(tempSess);
    VuExitCriticalSection();
    return true;
}

//removes a dangling to open a permanent one (newSess)
//returns 1 if data was exchanged ok
// 0 if we had no dangling sessions or the session was not there
int RemoveDanglingSession(VuSessionEntity *newSess)
{
    int retval = 0;
    char buffer[100];

    // no dangling sessions
    if ( not DanglingSessionsList)
    {
        return retval;
    }

    VuEnterCriticalSection();
    // iterate over dangling sessions
    VuListIterator dsit(DanglingSessionsList);

    for (
        VuSessionEntity *session = (VuSessionEntity*)dsit.GetFirst();
        session not_eq NULL;
        session = (VuSessionEntity*)dsit.GetNext()
    )
    {
        VU_ADDRESS newAdd = newSess->GetAddress();
        VU_ADDRESS oldAdd = session->GetAddress();

        if (
            (session->OwnerId().creator_.value_ == CAPI_DANGLING_ID) or
            (newSess->OwnerId().creator_.value_ == session->OwnerId().creator_)
        )
        {
            // new sessions have no handle and old one does. exchange them
            if ((newSess->GetCommsHandle() == NULL) and (session->GetCommsHandle() not_eq NULL))
            {
                newSess->SetCommsHandle(
                    session->GetCommsHandle(), F4CommsMaxUDPMessageSize, F4CommsIdealUDPPacketSize
                );
                sprintf(buffer, "%s UDP", ((FalconSessionEntity*)newSess)->GetPlayerCallsign());
                com_API_set_name(newSess->GetCommsHandle(), buffer);
                // inherit status from session
                newSess->SetCommsStatus(session->GetCommsStatus());
                session->SetCommsHandle(NULL);
            }

            if ((newSess->GetReliableCommsHandle() == NULL) and (session->GetReliableCommsHandle() not_eq NULL))
            {
                newSess->SetReliableCommsHandle(
                    session->GetReliableCommsHandle(), F4CommsMaxTCPMessageSize, F4CommsIdealTCPPacketSize
                );
                sprintf(buffer, "%s RUDP", ((FalconSessionEntity*)newSess)->GetPlayerCallsign());
                com_API_set_name(newSess->GetReliableCommsHandle(), buffer);
                // inherity status from session
                newSess->SetReliableCommsStatus(session->GetReliableCommsStatus());
                session->SetReliableCommsHandle(NULL);
            }

            // remove old dangling session
            DanglingSessionsList->Remove(session);
            // sfr: no need to delete this anymore, since its derefed from list
            //delete session;
            retval = 1;
            break;
        }
    }

    VuExitCriticalSection();
    return retval;
}

int UpdateDanglingSessions(void)
{
    if (gConnectionStatus)
    {
        VuEnterCriticalSection();
        VuListIterator dsit(DanglingSessionsList);
        VuSessionEntity *session;
        int count = 0;

        for (session = (VuSessionEntity*)dsit.GetFirst(); session; session = (VuSessionEntity*)dsit.GetNext())
        {
            count += session->GetMessages();
            // attempt to send one packet of each type
            session->FlushOutboundMessageBuffer();
        }

        VuExitCriticalSection();
        return count;
    }

    return 0;
}

// =====================================
// COMMS callbacks (registering handles)
// =====================================

// Callback from comms to register a tcp handle
// This gets called when someone sent a message to our TCP listen socket
#if 0
void TcpAcceptCallback(ComAPIHandle ch)
{
    ulong ipaddr;
    VuSessionEntity* s;
    VuSessionsIterator siter(vuGlobalGroup);

    // need to find session...
    ipaddr = ComAPIQuery(ch, COMAPI_CONNECTION_ADDRESS);

    // map ipaddr to session
    for (s = siter.GetFirst(); s; s = siter.GetNext())
    {
        if (s->Id().creator_.value_ == ipaddr)
        {
            MonoPrint("TcpAcceptCallback connection -- made to 0x%x ch = %p\n", ipaddr, ch);
            s->SetReliableCommsHandle(ch, F4CommsMaxTCPMessageSize, F4CommsIdealTCPPacketSize);
            s->SetReliableCommsStatus(VU_CONN_ACTIVE);
            // Request global entities from this guy
            VuMessage *req = new VuGetRequest(VU_GET_GLOBAL_ENTS, s);
            req->RequestReliableTransmit();
            VuMessageQueue::PostVuMessage(req);
            return;
        }
    }

    MonoPrint("TcpAcceptCallback error -- couldn't find session 0x%x\n", ipaddr);
    // should we disconnect here?
}

// This gets called as a result of us calling ComTCPOpenConnect (attempting to connect TCP)
// ret == 0 means success
void TcpConnectCallback(ComAPIHandle ch, int ret)
{
    ulong ipaddr;

    ipaddr = ComAPIQuery(ch, COMAPI_CONNECTION_ADDRESS);

    MonoPrint("Calling TcpConnectCallback.\n");

    if (ret not_eq 0)
    {
        MonoPrint("RequestConnection failed %d, will retry\n", ret);
        return;
    }

    // Check if we're connecting to a server first...
    if (FalconConnectionType == FCT_SERVER_AVAILABLE)
    {
        FalconGlobalTCPHandle = ch;

        if (ch)
        {
            FalconServerTCPStatus = VU_CONN_ACTIVE;

            if (gConnectionStatus == F4COMMS_PENDING)
                F4CommsConnectionCallback(F4COMMS_CONNECTED);
        }
        else
        {
            FalconServerTCPStatus = VU_CONN_INACTIVE; // This would be bad..
            F4CommsConnectionCallback(F4COMMS_ERROR_COULDNT_CONNECT_TO_SERVER);
        }

        return;
    }

    // Otherwise, look for the correct session
    VuEnterCriticalSection();
    VuSessionEntity* s;
    VuSessionsIterator siter(vuGlobalGroup);

    for (s = siter.GetFirst(); s; s = siter.GetNext())
    {
        if (s->Id().creator_.value_ == ipaddr)
        {
            RemoveDanglingSession(s);

            if (ch and s->GetCommsHandle() == ch)
                s->SetCommsStatus(VU_CONN_ACTIVE);

            if (ch and s->GetReliableCommsHandle() == ch)
                s->SetReliableCommsStatus(VU_CONN_ACTIVE);

            VuExitCriticalSection();

            if (gConnectionStatus == F4COMMS_PENDING)
                F4CommsConnectionCallback(F4COMMS_CONNECTED);

            // Request global entities from this guy
            VuMessage *req = new VuGetRequest(VU_GET_GLOBAL_ENTS, s);
            req->RequestReliableTransmit();
            VuMessageQueue::PostVuMessage(req);
            return;
        }
    }

    // We're connected (although we're not yet ready to send)
    if (gConnectionStatus == F4COMMS_PENDING)
    {
        F4CommsConnectionCallback(F4COMMS_CONNECTED);
    }

    // Add this connection to our "dangling connection" list
    AddDanglingSession(NULL, ch, ipaddr, ipaddr);
    VuExitCriticalSection();
}

// This gets called as a result of us calling ComDPLAYOpen and getting a modem connection
// ret == 0 means success
void ModemConnectCallback(ComAPIHandle ch, int ret)
{
    ulong ipaddr;

    ipaddr = ComAPIQuery(ch, COMAPI_CONNECTION_ADDRESS);

    ShiAssert(ch == FalconGlobalUDPHandle); // We should only have one connection

    // need to find session... There should be only two (us and them)
    VuEnterCriticalSection();
    VuSessionEntity* s;
    VuSessionsIterator siter(vuGlobalGroup);

    for (s = siter.GetFirst(); s; s = siter.GetNext())
    {
        if (s not_eq FalconLocalSession)
        {
            if (ret == 0)
            {
                MonoPrint("ModemConnectCallback invoked: connected\n");
                s->SetReliableCommsHandle(ch, F4CommsMaxTCPMessageSize, F4CommsIdealTCPPacketSize);
                s->SetReliableCommsStatus(VU_CONN_ACTIVE);
                // Request global entities from this guy
                VuMessage *req = new VuGetRequest(VU_GET_GLOBAL_ENTS, s);
                req->RequestReliableTransmit();
                VuMessageQueue::PostVuMessage(req);

                if (gConnectionStatus == F4COMMS_PENDING)
                    F4CommsConnectionCallback(F4COMMS_CONNECTED);
            }
            else
            {
                MonoPrint("ModemConnectCallback invoked: bad connection.\n");
                s->SetReliableCommsHandle(NULL);
                s->SetReliableCommsStatus(VU_CONN_INACTIVE);
            }

            VuExitCriticalSection();
            return;
        }
    }

    if (gConnectionStatus == F4COMMS_PENDING)
        F4CommsConnectionCallback(F4COMMS_CONNECTED);

    // Add this connection to our "dangling connection" list
    AddDanglingSession(NULL, ch,  ipaddr, ipaddr);
    MonoPrint("ModemConnectCallback invoked: saving handle..\n");
    VuExitCriticalSection();
}
#endif

// ====================================
// Time synchonizing stuff
// ====================================

// This routine will determine the proper time compression to run at after polling
// all sessions in the game.
//why are we taking force as a parameter when we aren't using it?
void ResyncTimes()
{
    int count,  best_comp;
    VuGroupEntity *g = FalconLocalGame;
    VuEnterCriticalSection();
    VuSessionsIterator sit(g);
    FalconSessionEntity *session;

    best_comp = 1;

    if (gTimeModeServer or g_bServer)
    {
        session = (FalconSessionEntity*)sit.GetFirst();
        count = 0;

        while (session)
        {
            if ( not session->IsLocal())
            {
                count ++;
                best_comp = session->GetReqCompression();
                break;
            }

            session = (FalconSessionEntity*)sit.GetNext();
        }

        if (count == 0)
        {
            best_comp = FalconLocalSession->GetReqCompression();
        }
    }
    else
    {
        best_comp = FalconLocalSession->GetReqCompression();
    }

    // KCK: Currently, everyone polls locally, but only sets time compression upon
    // notice from the host. We may prefer to send the host's poll data with the timing
    // message.
    remoteCompressionRequests = 0;

    session = (FalconSessionEntity*)sit.GetFirst();

    while (session)
    {
        if (gTimeModeServer or g_bServer)
        {
            if (session->IsLocal())
            {
                session = (FalconSessionEntity *) sit.GetNext();
                continue;
            }
        }

        if (session->GetReqCompression() > 1 and session->GetReqCompression() < best_comp)
        {
            best_comp = session->GetReqCompression();
            remoteCompressionRequests or_eq 1 << (best_comp - 1);
        }

        if (session->GetReqCompression() < 1 and session->GetReqCompression() > best_comp)
        {
            best_comp = session->GetReqCompression();
            remoteCompressionRequests or_eq REMOTE_REQUEST_PAUSE;
        }

        if (session->GetReqCompression() == 1)
        {
            best_comp = session->GetReqCompression();
        }

        if (session->GetReqCompression() == 0 and best_comp > 1)
        {
            best_comp = 1;
        }

        session = (FalconSessionEntity*)sit.GetNext();
    }

    VuExitCriticalSection();

    if (FalconLocalGame and FalconLocalGame->IsLocal())
    {
        lastStartTime = vuxRealTime;

        // Let the Campaign adjust itself to the target compression ratio
        targetCompressionRatio = best_comp;

        //MonoPrint ("Sending Timing Message %08x %d\n", vuxGameTime, targetCompressionRatio);
        FalconTimingMessage *msg = new FalconTimingMessage(FalconNullId, FalconLocalGame);
        msg->RequestOutOfBandTransmit();
        FalconSendMessage(msg, TRUE);
    }
}

// Time stamp function used by Comms and Vu to try and reduce latency in time stamping
ulong TimeStampFunction()
{
    return vuxRealTime;
}

//////////////////////////////////
// VUX STUFF
//////////////////////////////////

bool VuxAddDanglingSession(VU_ID owner, VU_ADDRESS address)
{
    return AddDanglingSession(owner, address);
}


// ==========================
// COMMS handle creation
// ==========================
// Set up a handle to communicate via UDP with everyone in this group.
int VuxGroupConnect(VuGroupEntity *group)
{
    char
    buffer[100],
           *name;

    if (group->IsGame())
    {
        name = ((VuGameEntity*)group)->GameName();
        MonoPrint("Connecting to game: %s\n", ((VuGameEntity*)group)->GameName());
    }
    else
    {
        name = group->GroupName();
        MonoPrint("Connecting to group: %s\n", group->GroupName());
    }

    // Check for existing connections
    if ( not group->GetCommsHandle())
    {
        if ( not (FalconConnectionProtocol bitand FCP_UDP_AVAILABLE) and not (FalconConnectionProtocol bitand FCP_SERIAL_AVAILABLE))
        {
            // No udp connections available
            group->SetCommsHandle(NULL);
            group->SetCommsStatus(VU_CONN_INACTIVE);
        }
        else if (FalconConnectionType bitand FCT_SERIAL_AVAILABLE)
        {
            group->SetCommsHandle(NULL); // We'll inherit from our global group
            group->SetCommsStatus(VU_CONN_ACTIVE);
        }
        else if (FalconConnectionType bitand FCT_SERVER_AVAILABLE and FalconGlobalUDPHandle)
        {
            // Point us to our server's UDP connection
            group->SetCommsHandle(NULL); // We'll inherit from our global group
            group->SetCommsStatus(VU_CONN_ACTIVE);
        }
        else if (FalconConnectionType bitand FCT_BCAST_AVAILABLE and FalconGlobalUDPHandle)
        {
            // Since we have broadcast available, pass our broadcast handle
            group->SetCommsHandle(NULL); // We'll inherit from our global group
            group->SetCommsStatus(VU_CONN_ACTIVE);
        }
        else if (FalconConnectionType bitand FCT_PTOP_AVAILABLE)
        {
            // Point to Point only - Create a new comms group which we will add shit to.
            sprintf(buffer, "%s UDP", name);
            MonoPrint("CreateGroup %s\n", buffer);
            com_API_handle gh = ComAPICreateGroup(buffer, F4CommsMaxUDPMessageSize, 0);
            group->SetCommsHandle(gh, F4CommsMaxUDPMessageSize, F4CommsIdealUDPPacketSize);

            if (gh)
                group->SetCommsStatus(VU_CONN_ACTIVE);
            else
                group->SetCommsStatus(VU_CONN_INACTIVE);
        }
        else
        {
            // No communication available.
            group->SetCommsHandle(NULL);
            group->SetCommsStatus(VU_CONN_INACTIVE);
        }
    }

    if ( not group->GetReliableCommsHandle())
    {
        if ( not (FalconConnectionProtocol bitand FCP_TCP_AVAILABLE) and not (FalconConnectionProtocol bitand FCP_SERIAL_AVAILABLE) and not (FalconConnectionProtocol bitand FCP_RUDP_AVAILABLE))
        {
            // No reliable connections available
            group->SetReliableCommsHandle(NULL);
            group->SetReliableCommsStatus(VU_CONN_INACTIVE);
        }
        else if (FalconConnectionType bitand FCT_SERIAL_AVAILABLE)
        {
            group->SetCommsHandle(NULL); // We'll inherit from our global group
            group->SetCommsStatus(VU_CONN_ACTIVE);
        }
        else if (FalconConnectionType bitand FCT_SERVER_AVAILABLE and FalconGlobalTCPHandle)
        {
            // Point us to our server's tcp connection
            group->SetCommsHandle(NULL); // We'll inherit from our global group
            group->SetReliableCommsStatus(VU_CONN_ACTIVE);
        }
        else if (FalconConnectionType bitand FCT_PTOP_AVAILABLE)
        {
            // Point to Point only - Create a new comms group which we will add shit to.
            sprintf(buffer, "%s RUDP", name);
            MonoPrint("CreateGroup %s\n", buffer);
            com_API_handle gh = ComAPICreateGroup(buffer, F4CommsMaxTCPMessageSize, 0);
            group->SetReliableCommsHandle(gh, F4CommsMaxTCPMessageSize, F4CommsIdealTCPPacketSize);

            if (gh)
                group->SetReliableCommsStatus(VU_CONN_ACTIVE);
            else
                group->SetReliableCommsStatus(VU_CONN_INACTIVE);
        }
        else
        {
            // No reliable group communication available.
            group->SetReliableCommsHandle(NULL);
            group->SetReliableCommsStatus(VU_CONN_INACTIVE);
        }
    }

    if (gUICommsQ and group->IsGame())
        gUICommsQ->Add(_Q_GAME_ADD_, FalconNullId, group->Id());

    return 0;
}

void VuxGroupDisconnect(VuGroupEntity *group)
{
    com_API_handle ch;

    if (group->IsGame())
    {
        MonoPrint("Disconnecting to game: %s\n", ((VuGameEntity*)group)->GameName());
    }
    else
    {
        MonoPrint("Disconnecting to group: %s\n", group->GroupName());
    }

    // Disconnect UDP
    ch = group->GetCommsHandle();
    group->SetCommsHandle(NULL);
    group->SetCommsStatus(VU_CONN_INACTIVE);

    if (ch and ch not_eq FalconGlobalUDPHandle)
        ComAPIClose(ch);

    // Disconnect TCP
    ch = group->GetReliableCommsHandle();
    group->SetReliableCommsHandle(NULL);
    group->SetReliableCommsStatus(VU_CONN_INACTIVE);

    if (ch and ch not_eq FalconGlobalTCPHandle)
        ComAPIClose(ch);

    if (gUICommsQ and group->IsGame())
        gUICommsQ->Add(_Q_GAME_REMOVE_, FalconNullId, group->Id());
}


int VuxGroupAddSession(VuGroupEntity *group, VuSessionEntity *session)
{
    com_API_handle gh, sh;

    if (g_bVoiceCom)
    {
        if (strcmpi(g_strVoiceHostIP, ""))
        {
            g_ipadress = g_strVoiceHostIP;
        }

        if (
            (gConnectionStatus == F4COMMS_CONNECTED or
             (g_ipadress and not strcmpi(g_ipadress, "0.0.0.0"))) and 
 not stoppingvoice and not g_pDPServer and not g_pDPClient and (g_ipadress)
        )
        {
            startupvoice(g_ipadress);//me123
        }
    }

    // sfr bw control
    // adjusted only for games
    if (group->IsGame())
    {
        FalconGameEntity *game = static_cast<FalconGameEntity*>(group);

        if (session == vuLocalSessionEntity)
        {
            // adding ourselves to group
            if (game == vuPlayerPoolGroup)
            {
                ComAPIBWEnterState(CAPI_LOBBY_ST);
            }
            else
            {
                switch (game->gameType)
                {
                        //case game_PlayerPool:
                        // ComAPIBWEnterState(CAPI_LOBBY_ST);
                        //break;
                    case game_Dogfight:
                        ComAPIBWEnterState(CAPI_DF_ST);
                        break;

                    case game_TacticalEngagement:
                    case game_Campaign:
                        // here is different if we are host, for others its the same
                        ComAPIBWEnterState((game->OwnerId() == vuLocalSession) ? CAPI_CAS_ST : CAPI_CAC_ST);
                        break;

                    default:
                        ; // do nothing
                }
            }

            // if game is not ours, we need to update bw to reflect other players already in
            if (game->OwnerId() not_eq vuLocalSessionEntity->Id())
            {
                for (unsigned int players = game->SessionCount(); players > 0; --players)
                {
                    ComAPIBWPlayerJoined();
                }
            }
        }
        else if (game == vuLocalGame)
        {
            // adding others to group
            ComAPIBWPlayerJoined();
        }
    }

    gh = group->GetCommsHandle();
    sh = session->GetCommsHandle();

    if (gh and sh and gh not_eq sh)
    {
        ComAPIAddToGroup(gh, sh);
    }

    gh = group->GetReliableCommsHandle();
    sh = session->GetReliableCommsHandle();

    if (gh and sh and gh not_eq sh)
    {
        ComAPIAddToGroup(gh, sh);
    }

    // UI Related
    if (gUICommsQ and group->IsGame())
    {
        gUICommsQ->Add(_Q_SESSION_ADD_, session->Id(), group->Id());
    }

    // Send FullUpdate for session if this is our game
    if (
        (group->IsGame()) and 
        (group->Id() == vuLocalSessionEntity->GameId()) and 
        (vuLocalSessionEntity.get() not_eq session)
    )
    {
        VuFullUpdateEvent *msg = new VuFullUpdateEvent(vuLocalSessionEntity.get(), session);
        msg->RequestReliableTransmit();
        VuMessageQueue::PostVuMessage(msg);
    }

    return 1;
}

int VuxGroupRemoveSession(VuGroupEntity *group, VuSessionEntity *session)
{
    // check if session is in group
    if ( not group->SessionInGroup(session))
    {
        return VU_NO_OP;
    }

    com_API_handle gh, sh;

    if (group->IsGame())
    {
        FalconGameEntity *game = static_cast<FalconGameEntity*>(group);

        if (session == vuLocalSessionEntity)
        {
            // were leaving game
            ComAPIBWEnterState(CAPI_LOBBY_ST);
        }
        else if (group == vuLocalGame)
        {
            // player left game were in
            ComAPIBWPlayerLeft();
        }
    }

    gh = group->GetCommsHandle();
    sh = session->GetCommsHandle();

    if (gh and sh and (gh not_eq sh))
    {
        ComAPIDeleteFromGroup(gh, sh);
    }

    // if (sh == FalconInitialUDPHandle and gh == FalconGlobalUDPHandle)
    // FalconInitialUDPHandle = NULL;
    gh = group->GetReliableCommsHandle();
    sh = session->GetReliableCommsHandle();

    if (gh and sh and gh not_eq sh)
    {
        ComAPIDeleteFromGroup(gh, sh);
    }

    if (gUICommsQ)
    {
        gUICommsQ->Add(_Q_SESSION_REMOVE_, session->Id(), group->Id());
    }

    // VWF 12/1/98: Added this to clean up player's flight when he leaves game
    if ((FalconLocalGame == group) and (FalconLocalGame->IsLocal()))
    {
        Flight flight = ((FalconSessionEntity*)session)->GetAssignedPlayerFlight();

        if (flight)
        {
            LeaveACSlot(
                ((FalconSessionEntity*)session)->GetAssignedPlayerFlight(),
                ((FalconSessionEntity*)session)->GetAssignedAircraftNum()
            );
            // Make sure this session doesn't have any old information
        }
    }

    // END VWF

    ((FalconSessionEntity*)session)->SetPlayerSquadron(NULL);
    ((FalconSessionEntity*)session)->SetPlayerFlight(NULL);
    ((FalconSessionEntity*)session)->SetPlayerEntity(NULL);
    ((FalconSessionEntity*)session)->SetAircraftNum(255);
    ((FalconSessionEntity*)session)->SetPilotSlot(255);
    ((FalconSessionEntity*)session)->SetAssignedAircraftNum(255);
    ((FalconSessionEntity*)session)->SetAssignedPilotSlot(255);
    ((FalconSessionEntity*)session)->SetAssignedPlayerFlight(NULL);

    return 1;
}

// Set up a handle to communicate this session.
int VuxSessionConnect(VuSessionEntity *session)
{
    // char buffer[100];
    int wait_for_connection = 0;

    // We only want to connect here during our initial insertion
    if (session->GameAction() not_eq VU_NO_GAME_ACTION)
    {
        return 0;
    }

    if (session == vuLocalSessionEntity)
    {
        // This is us.. We don't need to connect
        session->SetCommsHandle(NULL);
        session->SetCommsStatus(VU_CONN_INACTIVE);
        session->SetReliableCommsHandle(NULL);
        session->SetReliableCommsStatus(VU_CONN_INACTIVE);
        return 0;
    }

    // add dangling session now if it doesnt exist
    AddDanglingSession(session->Id(), session->GetAddress());
    RemoveDanglingSession(session);

    // now create the reliable handle if we dont have one
    if (session->GetReliableCommsHandle() == NULL)
    {
        char buffer[20];
        sprintf(buffer, "%s RUDP", ((FalconSessionEntity*)session)->GetPlayerCallsign());
        //sfr: vu change converts
        VU_ADDRESS add = session->GetAddress();
        com_API_handle rudpHandle = ComRUDPOpen(
                                      buffer,
                                      F4CommsMaxTCPMessageSize,
                                      vuxWorldName,
                                      vuLocalSessionEntity->GetAddress().reliableRecvPort,
                                      add.reliableRecvPort,
                                      add.ip,
                                      session->Id().creator_.value_,
                                      F4CommsIdealPacketSize
                                  );
        session->SetReliableCommsHandle(rudpHandle, F4CommsMaxTCPMessageSize, F4CommsIdealTCPPacketSize);

        if (rudpHandle)
        {
            session->SetReliableCommsStatus(VU_CONN_ACTIVE);
        }
        else
        {
            session->SetReliableCommsStatus(VU_CONN_INACTIVE);
        }
    }

    // Add to the global group
    VuxGroupAddSession(vuGlobalGroup, session);

    //sfr: this can be a duplicated message (see function VuxGroupAddSession)
    {
        //send ourselves to session we are opening connection
        VuMessage *req = new VuFullUpdateEvent(vuLocalSessionEntity.get(), session);
        req->RequestOutOfBandTransmit();
        //req->Send();
        VuMessageQueue::PostVuMessage(req);
    }
    {
        // and request all global ents
        VuMessage *req = new VuGetRequest(VU_GET_GLOBAL_ENTS, session);
        //req->RequestReliableTransmit();
        req->RequestOutOfBandTransmit();
        //req->Send();
        VuMessageQueue::PostVuMessage(req);
    }


    return 0;
}

void VuxSessionDisconnect(VuSessionEntity *session)
{

    if (session == FalconLocalSession)
        return;

    // We only want to do this if we're leaving a game (i.e: This session is going away)
    if (session->GameAction() not_eq VU_LEAVE_GAME_ACTION)
        return;


    Flight playerFlight = (Flight)((FalconSessionEntity*)session)->GetAssignedPlayerFlight();
    int acnumber = ((FalconSessionEntity*)session)->GetAssignedAircraftNum();

    if (g_bACPlayerCTDFix and playerFlight and FalconLocalGame->IsLocal()) // only the host...
    {
        FalconPlayerStatusMessage *msg = new FalconPlayerStatusMessage(((FalconSessionEntity*)session)->Id(), FalconLocalGame);
        SimBaseClass *playerEntity = (SimBaseClass*)((FalconSessionEntity*)session)->GetPlayerEntity();

        // first change the deag owner of our disconnected player back to the host
        /* if (playerEntity)
         {
         FalconSimCampMessage *simmsg = new FalconSimCampMessage (playerEntity->Id(), FalconLocalGame); // target);
         simmsg->dataBlock.from = FalconLocalGame->OwnerId();
         simmsg->dataBlock.message = FalconSimCampMessage::simcampChangeOwner;
         FalconSendMessage(simmsg);
         }
         */
        if (((FalconSessionEntity*)session)->GetPlayerFlight())
        {
            msg->dataBlock.campID = ((FalconSessionEntity*)session)->GetPlayerFlight()->GetCampID();
        }
        else
        {
            msg->dataBlock.campID = 0;
        }

        if (playerEntity)
        {
            msg->dataBlock.playerID         = playerEntity->Id();
        }

        _tcscpy(msg->dataBlock.callsign, ((FalconSessionEntity*)session)->GetPlayerCallsign());

        msg->dataBlock.side             = ((FalconSessionEntity*)session)->GetCountry();
        msg->dataBlock.pilotID          = ((FalconSessionEntity*)session)->GetPilotSlot();
        msg->dataBlock.vehicleID = ((FalconSessionEntity*)session)->GetAircraftNum();
        msg->dataBlock.state            = PSM_STATE_LEFT_SIM;

        FalconSendMessage(msg, TRUE);
        ((FalconSessionEntity*)session)->SetFlyState(FLYSTATE_IN_UI);

        // and now we need to do some hacking... the current code doesn't really support to change ownership
        // of single aircraft


        // Get the Aircraftclass entity
        AircraftClass *playerAircraft = NULL;

        if (playerFlight)
            playerAircraft = (AircraftClass *)playerFlight->GetComponentEntity(acnumber);

        // when we still have one, just make local to the host
        if (playerAircraft)
        {
            playerAircraft->MakeLocal();
        }
    }

    // 2002-04-14 MN remove the disconnected (CTD'ed, Internet connection lost) player from its slot so it can be occupied again
    LeaveACSlot(((FalconSessionEntity*)session)->GetAssignedPlayerFlight(), ((FalconSessionEntity*)session)->GetAssignedAircraftNum());

    // Remove from the global group
    VuxGroupRemoveSession(vuGlobalGroup, session);

    if (session->GetCommsStatus() not_eq VU_CONN_INACTIVE or session->GetReliableCommsStatus() not_eq VU_CONN_INACTIVE)
    {
        MonoPrint("Disconnecting to session: %s\n", ((FalconSessionEntity*)session)->GetPlayerCallsign());
    }

    /* me123 commented this out
       it prevents us from removing the session from other lists then the global group.
       it should not matter since the session will be removed totaly right after this.
    // Disconnect UDP
    ch = session->GetCommsHandle();
    session->SetCommsHandle(NULL);
    session->SetCommsStatus(VU_CONN_INACTIVE);
    if (session and ch and ch not_eq FalconGlobalUDPHandle)
    ComAPIClose(ch);
    // Disconnect TCP
    ch = session->GetReliableCommsHandle();
    session->SetReliableCommsHandle(NULL);
    session->SetReliableCommsStatus(VU_CONN_INACTIVE);
    if (session and ch and ch not_eq FalconGlobalTCPHandle)
    ComAPIClose(ch);
    */
}


void VuxAdjustLatency(VU_TIME, VU_TIME)
{
    // KCK NOTE: This is stubbed 'cause we're not using VU's latency checking
    // SynchronizeTime(vuLocalSessionEntity->Group());
    // MonoPrint("Called VuxAdjustLatency(%d, %d)\n", t1, t2);
}
