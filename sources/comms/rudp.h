#ifndef _RUDP_H
#define _RUDP_H

/** @file rudp.h 
* rudp related heder file 
* @author sfr
*/

#include "capi.h"

#ifdef __cplusplus
extern "C"{
#endif

/** set up RUDP port, send will  be random
* @param rport RUDP receive port, host order
*/
void ComRUDPSetup(unsigned short rport);

ComAPIHandle ComRUDPOpen(
	char *name,                 // name of this comm, usually "callsign RUDP"
	int buffersize,
	char *gamename,             // game name 
	unsigned short localPort,   // local port (host order)
	unsigned short remotePort,  // where peer receives reliable data (host order)
	unsigned long IPaddress,    // peer ip address. If == CAPI_DANGLING_IP, this is a receive only comm
	unsigned long id,           // id of the owner of this comm
	int idealpacketsize       
);

#ifdef __cplusplus
}
#endif

#endif