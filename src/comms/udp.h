#ifndef _UDP_H
#define _UDP_H

/** @file udp.h
* udp related header file 
* @author sfr
*/

#include "capi.h"

#ifdef __cplusplus
extern "C"{
#endif

/** set up UDP ports. Send will be random
* @param port UDP receive port, host order
*/
void ComUDPSetup(unsigned short rport);

ComAPIHandle ComUDPOpen(
	char *name, // name of this comm, usuallly callsign UDP
	int buffersize, 
	char *gamename,
	unsigned short localUdpPort, // port where we receive data from this entity
	unsigned short remoteUdpPort,  // port where this entity receives udp data
	unsigned long IPaddress, // peer ip, if == CAPI_DANGLING_IP, this is a receive only comm
	unsigned long id // id of the owner
);

#ifdef __cplusplus
}
#endif

#endif