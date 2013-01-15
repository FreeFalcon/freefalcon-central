/***************************************************************************\
    Transport.cpp
    Scott Randolph
    May 30, 1997

    Simple WinSock UDP broadcast transport service.
\***************************************************************************/
#include <windows.h>
#include <winsock.h>
#include "shi\ShiError.h"
#include "Transprt.h"


/*
#ifndef LOOPBACK_TEST

static SOCKET		sock;
static sockaddr_in	address;

void SetupTransport( WORD port )
{
	WORD		wVersionRequested;  
	WSADATA		wsaData; 
	int			result; 



	// Open WinSock
	wVersionRequested = MAKEWORD(1, 1); 
	result = WSAStartup(wVersionRequested, &wsaData);
	if (result != 0) {
		ShiError( "Transport:  We couldn't load WinSock!" ); 
	}
	if ( LOBYTE( wsaData.wVersion ) != 1 || HIBYTE( wsaData.wVersion ) != 1 ) { 
		WSACleanup(); 
		ShiError( "Transport:  We couldn't find a 1.1 compatable WinSock!" ); 
	} 


	// Create a UDP socket
	sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if (sock == INVALID_SOCKET) {
		ShiError( "Transport:  Failed to create socket" );
	}

	// Enable broadcast on this socket
	int		setTrue = 1;
	result = setsockopt( sock, SOL_SOCKET, SO_BROADCAST, (char*)&setTrue, sizeof(setTrue) );
	ShiAssert( result == 0 );

	// Bind our socket to our local address and magic port
	memset( &address, 0, sizeof(address) );
	address.sin_family				= AF_INET;
	address.sin_addr.S_un.S_addr	= INADDR_ANY;
	address.sin_port				= port;
	result = bind( sock, (sockaddr*)&address, sizeof(address) );
	ShiAssert( result == 0 );

	// Now set the address to the broadcast address for use in Send()
	address.sin_addr.S_un.S_addr	= INADDR_BROADCAST;

}


void CleanupTransport( void )
{
	// Close our socket
	closesocket( sock );

	// Close WinSock
	WSACleanup(); 
}


// Send out one datagram
unsigned Send( void* data, unsigned size )
{
	int result;

	result = sendto( sock, (char*)data, size, 0, (sockaddr*)&address, sizeof(address) );
	if (result == SOCKET_ERROR) {
		ShiError("Transport:  Send failed!");
	}

	return result;
}


// Read out one datagram (at most "size" bytes)
unsigned Receive( void* data, unsigned size )
{
	int result;

	result = recv( sock, (char*)data, size, 0 );
	if (result == SOCKET_ERROR) {
		ShiError("Transport:  Recv failed!");
	}

	return result;
}


// Return TRUE if data is read for reading
BOOL DataReady( void )
{
	int		result;
	fd_set	selectSet;
	timeval	time = { 0, 0 };

	FD_ZERO( &selectSet );
	FD_SET(  sock, &selectSet );

	result = select( 1, &selectSet, NULL, NULL, &time );
	if (result == SOCKET_ERROR) {
		ShiError("Transport:  Select failed!");
	}

	return result;
}

#else

typedef struct
{
	unsigned	size;
	BYTE		*ptr;
} XR_STRUCT;

int numInBuffer;
XR_STRUCT *firstXR, *lastXR;
XR_STRUCT gBufs[ 50 ];

void SetupTransport( WORD bufsize )
{
	int i;

	for ( i = 0; i < 50; i++ )
	{
		gBufs[i].size = 0;
//		gBufs[i].ptr = (BYTE *)malloc( bufsize );
		gBufs[i].ptr = new BYTE[bufsize];
		if ( !gBufs[i].ptr )
			ShiError(" Loopback test: buffer alloc failed" );
	}
	numInBuffer = 0;
	firstXR = lastXR = &gBufs[0];
}


void CleanupTransport( void )
{
	int i;

	for ( i = 0; i < 50; i++ )
	{
		delete [] gBufs[i].ptr;
	}
}


// Send out one datagram
unsigned Send( void* data, unsigned size )
{
	if ( numInBuffer == 50 )
		return 0;

	lastXR->size = size;
	memcpy( lastXR->ptr, (BYTE *)data, size );
	lastXR++;
	numInBuffer++;

	return size;

}


// Read out one datagram (at most "size" bytes)
unsigned Receive( void* data, unsigned size )
{
	if ( numInBuffer == 0 || firstXR == lastXR )
		return 0;
	memcpy( data, (BYTE *)firstXR->ptr, firstXR->size );

	size = firstXR->size;
	firstXR++;

	return size;

}

// Return TRUE if data is read for reading
BOOL DataReady( void )
{
	if ( numInBuffer == 0 || firstXR == lastXR )
		return FALSE;

	return TRUE;
}

void ResetTransport( void )
{
	numInBuffer = 0;
	firstXR = lastXR = &gBufs[0];
}

#endif
*/