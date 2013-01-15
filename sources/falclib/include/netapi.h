/* ---------------------------------------------------------------------
 * NetApi.h -- Network Include File
 * ---------------------------------------------------------------------
 * Data structures and protoypes used by the game `applications' or 
 * higher-level subsystems in accessing the network subsystem. 
 * ---------------------------------------------------------------------
 * David Sarnoff Research Center - PROPRIETARY INFORMATION
 * Copyright 1993-94, David Sarnoff Research Center, All Rights Reserved
 * ---------------------------------------------------------------------
 */ 

#ifndef   NETAPI_H
#define   NETAPI_H

#include "apitypes.h"
#include "vu2\comms\comcore.h"

#ifdef API_VERSION
#define NET_VERSION API_VERSION
#else
#define NET_VERSION 1.3
#endif

#define  NET_MAXIMUM_STATIONS               6

/* Net Function return values:  */

#define  NET_ERROR_NONE                     0
#define  NET_ERROR_NETWORK_DISRUPTED        1
#define  NET_ERROR_NETWORK_RESTORED         2
#define  NET_ERROR_NOT_OPEN                -1
#define  NET_ERROR_INVALID_MESSAGE_SIZE    -2
#define  NET_ERROR_INVALID_STATION_ID      -3
#define  NET_ERROR_INVALID_GROUP_ID        -4
#define  NET_ERROR_RECEIVER_OVERRUN        -5
#define  NET_ERROR_DUPLICATE_STATION_ID    -6

#define  NET_POLL_NO_CHANGE                 0
#define  NET_POLL_MESSAGE_RECEIVED          3
#define  NET_POLL_STATUS_CHANGE             4

#define  NET_SEND_TRANSMIT_BUFFER_FULL      5

#define  NET_TO_ALL_STATIONS               -1
#define  NET_TO_GAME_STATIONS              16

typedef  struct {
    int    stationId;
    char   gameId[VU_GAME_IDENTIFICATION_LENGTH];
    int    maxMessageSize;
}  NetOpenRequest;

typedef  struct {
    int    messageDestination;
    VU_BYTE  *messageDataBuffer;
    int    messageDataCount;
    int    messagePriority;
}  NetSendRequest;

typedef  struct {
    int    messageOriginator;
    VU_BYTE  *messageDataBuffer;
    int    messageDataCount;
    int    messageCount;
}  NetPollRequest;

typedef  struct {
    int              stationCount;
    ComStationDesc  *stationTablePtr;
}  NetInqRequest;

int NetBegin(float version, char mode);

/**************************************************
 * NetOpen request structure typedef and prototype:
 *
 *     stationId       is the returned value (1 - 6) of the unique network id
 *                     assigned to this station for the duration of this
 *                     session;
 *     gameId          is the specified name of the application game which is
 *                     active for the duration of this session;
 *     maxMessageSize  is the size of the longest message to be transmitted or
 *                     received during this session.
 */

int NetOpen(NetOpenRequest *openReq);

/**************************************************
 * NetSend request structure typedef and prototype:
 *
 *    messageDestination  is the desired routing of this message; the permitted 
 *                         routings are:
 *                             to a specific station
 *                                 (1 <= messageDestination <= 6)
 *                             to a specific group
 *                                 (17 <= messageDestination <= 21)
 *                             to all stations
 *                                 (messageDestination = NET_TO_ALL_STATIONS)
 *                             to all stations playing the this game
 *                                 (messageDestination = NET_TO_GAME_STATIONS);
 *     messageDataBuffer   is a pointer to the buffer containing the message to
 *                         be transmitted;
 *     messageDataCount    is the length, in bytes, of the message;
 *     messagePriority     is the priority of the message, t.b.d.
 */

int NetSend(NetSendRequest *sendReq);

/**************************************************
 * NetPoll request structure typedef and prototype:
 *
 *     messageOriginator   is the returned id (1 - 6) of the station which
 *                         originated the message;
 *     messageDataBuffer   is a pointer to the buffer into which a received
 *                         message, if one is available, will be copied;
 *     messageDataCount    is the returned length, in bytes, of the received
 *                         message;
 *     messageCount        is the number of received messages remaining.
 */

int NetPoll(NetPollRequest *pollReq);

/*****************************************************
 * NetInquire request structure typedef and prototype:
 *
 *     stationCount        is the returned count of the active stations; i.e.,
 *                         the number of elements copied to the station table; 
 *     stationTablePtr     is a pointer to the table into which the active
 *                         station descriptors will be copied;
 */

int NetInquire(NetInqRequest *inqReq);

int NetOpen(NetOpenRequest *openReq);
int  NetEnd(void);
int  NetClose(void);

ushort GetTempStationID( ushort );

#endif   /* NETAPI_H */
