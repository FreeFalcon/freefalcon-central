/***************************************************************************\
    Transport.h
    Scott Randolph
    May 30, 1997

    Simple WinSock UDP broadcast transport service.
\***************************************************************************/
#ifndef _TRANSPORT_H_
#define _TRANSPORT_H_


// Functions for public use
void		SetupTransport( WORD port );
void		CleanupTransport( void );

unsigned	Send( void* data, unsigned size );
unsigned	Receive( void* data, unsigned size );

BOOL		DataReady( void );

// #define LOOPBACK_TEST
#ifdef LOOPBACK_TEST
void		ResetTransport( void );
#endif

#endif // _TRANSPORT_H_
