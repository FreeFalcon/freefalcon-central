/***************************************************************************\
    TalkIO.h
    Scott Randolph
    May 30, 1997

    Direct Sound speech transmission layer.
\***************************************************************************/
#ifndef _TALKIO_H_
#define _TALKIO_H_
#include <dsound.h>

typedef enum	TalkIOMode	{ Transmitting, Receiving };


// Functions for public use
void	SetupTalkIO( LPDIRECTSOUNDCAPTURE inDevice, LPDIRECTSOUND outDevice, WAVEFORMATEX *audioFormat );
void	CleanupTalkIO( void );

void	BeginTransmission();
void	EndTransmission();

BOOL DSErrorCheck( HRESULT result );

// Private functions
void __cdecl TalkIOloop( void *args );


#endif // _TALKIO_H_
