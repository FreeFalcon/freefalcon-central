#ifndef _F4COMMS_STUFF_H
#define _F4COMMS_STUFF_H

#include "vusessn.h"
#include "falclist.h"

extern ComAPIHandle vuxComHandle;
extern ComAPIHandle tcpListenHandle;

enum FalconConnectionTypes {
  FCT_NoConnection=0,
  FCT_ModemToModem,						// Modem to modem only
  FCT_NullModem,
  FCT_LAN,								// LAN or simulated LAN (kali)
  FCT_WAN,								// Internet or modem to ISP
  FCT_Server,							// Internet,modem or LAN to server
  FCT_TEN,
  FCT_JetNet,
};

// ========================================================================
// Message sizing variables
// ========================================================================

// Ideal packet size we'll send over the wire;
extern int F4CommsIdealPacketSize;
// Corrisponding content sizes
extern int F4CommsIdealTCPPacketSize;
extern int F4CommsIdealUDPPacketSize;
// Corrisponding messages sizes
extern int F4CommsMaxTCPMessageSize;
extern int F4CommsMaxUDPMessageSize;
// Maximum sized message vu can accept
extern int F4VuMaxTCPMessageSize;
extern int F4VuMaxUDPMessageSize;
// Maximum sized packet vu will pack messages into
extern int F4VuMaxTCPPackSize;
extern int F4VuMaxUDPPackSize;
// debug bandwidth limiters
extern int F4CommsBandwidth;
extern int F4CommsLatency;
extern int F4CommsDropInterval;
extern int F4CommsSwapInterval;
extern int F4SessionUpdateTime;
extern int F4SessionAliveTimeout;
extern int F4CommsMTU;  // More of Unz and  Boosters stuff
// ========================================================================
// Some defines 
// ========================================================================

// Protocols available to Falcon
#define FCP_UDP_AVAILABLE		0x01	// We have UDP available
#define FCP_TCP_AVAILABLE		0x02	// We have TCP available
#define FCP_SERIAL_AVAILABLE	0x04	// We have a serial (via modem or null modem) connection
#define FCP_MULTICAST_AVAILABLE	0x08	// True multicast is available
#define FCP_RUDP_AVAILABLE		0x10	// We have RUDP available

// Virtual connection types available to Falcon
#define FCT_PTOP_AVAILABLE		0x01	// We can send point to point messages to multiple machines
#define FCT_BCAST_AVAILABLE		0x02	// We can send broadcast (to world) messages
#define FCT_SERVER_AVAILABLE	0x04	// We are connecting to an exploder server
#define FCT_SERIAL_AVAILABLE	0x08	// We are connected via modem or null modem to one other machine

// Error codes returned from InitCommsStuff()
#define F4COMMS_CONNECTED                        1
#define F4COMMS_PENDING							 2		// Seems successfull, but we're waiting for a connection
#define	F4COMMS_ERROR_TCP_NOT_AVAILABLE			-1
#define F4COMMS_ERROR_UDP_NOT_AVAILABLE			-2
#define F4COMMS_ERROR_MULTICAST_NOT_AVAILABLE	-3
#define F4COMMS_ERROR_FAILED_TO_CREATE_GAME		-4
#define F4COMMS_ERROR_COULDNT_CONNECT_TO_SERVER	-5

class ComDataClass;

//sfr: globals defined in F4Comms.c
extern int FalconServerTCPStatus;
extern int FalconConnectionProtocol;
extern int FalconConnectionType;
extern int FalconConnectionDescription;
extern int gConnectionStatus;
extern int gTimeModeServer;
extern FalconPrivateList *DanglingSessionsList;
extern ComAPIHandle FalconTCPListenHandle;
extern ComAPIHandle FalconGlobalUDPHandle;
extern ComAPIHandle FalconGlobalTCPHandle;
extern int FalconServerTCPStatus;
extern int FalconConnectionProtocol;
extern int FalconConnectionType;
extern int FalconConnectionDescription;
extern int gConnectionStatus;
extern int gTimeModeServer;
extern int g_b_forcebandwidth;
extern char* g_ipadress;


// ========================================================================
// Function prototypes
// ========================================================================

int InitCommsStuff (ComDataClass *comData);

int EndCommsStuff (void);

BOOL SessionManagerProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

void SetupMessageSizes (int protocol);

void ResyncTimes();

//sfr: added these functions to header
void CleanupDanglingList(void);
//void AddDanglingSession(ComAPIHandle ch1, ComAPIHandle ch2, VU_SESSION_ID id, VU_ADDRESS add);
// returns true if dangling session sucessfully created
bool AddDanglingSession(VU_ID owner, VU_ADDRESS add);
int RemoveDanglingSession (VuSessionEntity *newSess);
int F4CommsConnectionCallback (int result);
//void FillSerialDataString(char serial_data[], ComDataClass *comData);
//void ModemConnectCallback(ComAPIHandle ch, int ret);
//void TcpConnectCallback(ComAPIHandle ch, int ret);
int CleanupComms (void);
void ShutdownCampaign (void);
ulong TimeStampFunction (void);

#endif
