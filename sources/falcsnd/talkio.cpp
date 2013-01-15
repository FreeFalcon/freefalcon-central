/***************************************************************************\
    TalkIO.cpp
    Scott Randolph
    May 30, 1997

    Direct Sound speech transmission layer.
\***************************************************************************/
#include <windows.h>
#include <process.h>
#include "fsound.h"
#include "shi\ShiError.h"
#include "TalkIO.h"
#include "Transprt.h"

#if 0

#define DOWNSAMPLE_AMT			3
#define FLUSH_LOCAL_TRANSMISSIONS
#define NDEBUG
#define CHAT_USED


static const WORD	PORT_NUMBER = 3870;
static const DWORD	SAMPLE_TIME	= 100;							// Milliseconds

static HANDLE		threadHandle;
static BOOL			runFlag;
static HANDLE		wakeEvent;
static TalkIOMode	CurrentState;

static DSBUFFERDESC					outBufferDesc;
static DSCBUFFERDESC				inBufferDesc;
static LPDIRECTSOUNDCAPTUREBUFFER	inBuf;
static LPDIRECTSOUNDBUFFER			outBuf;
static IDirectSoundNotify			*inNotify;
static DSBPOSITIONNOTIFY			positions[3];

static DWORD soundChunkSize;
static BYTE  *compressorInput;
static DWORD compressorInputSize;
static DWORD readPosition;
static DWORD writePosition;
static DWORD bytesPerSecond;
static DWORD transmissionCount;
static DWORD packetCount;
static WAVEFORMATEX playbackFormat;
static BOOL firstPacketDelayed;
static DWORD outBufSize;

void	SetupTalkIO( IDirectSoundCapture *inDevice, IDirectSound *outDevice, WAVEFORMATEX *audioFormat )
{
	#ifdef CHAT_USED
	HRESULT	result;
	

	// playback format will be at a sample rate 1/4 of capture format
	// in 16 bit mono
	playbackFormat.wFormatTag		= WAVE_FORMAT_PCM;
	playbackFormat.nChannels		= 1;
	playbackFormat.nSamplesPerSec	= audioFormat->nSamplesPerSec/DOWNSAMPLE_AMT;
	playbackFormat.nAvgBytesPerSec	= playbackFormat.nSamplesPerSec * 2;
	playbackFormat.nBlockAlign		= 2;
	playbackFormat.wBitsPerSample	= 16;
	playbackFormat.cbSize			= 0;

	// Precompute our buffer sizes
	bytesPerSecond = audioFormat->nAvgBytesPerSec;
	soundChunkSize = bytesPerSecond * SAMPLE_TIME/1000;

	#ifdef LOOPBACK_TEST
	SetupTransport( (WORD)soundChunkSize );
	#else
	SetupTransport( PORT_NUMBER );
	#endif

	// Allocate our compressor input buffer
//	compressorInput = (BYTE*)malloc( soundChunkSize * 2 );
	compressorInput = new BYTE[soundChunkSize * 2];
	ShiAssert(compressorInput);

	// Setup our signaling event
	wakeEvent = CreateEvent( NULL, FALSE, FALSE, "TalkIOwake" );
	ShiAssert( wakeEvent );


	// Create the output buffer
	OutputDebugString("Creating DSound secondary output buffer\n");
	memset( &outBufferDesc, 0, sizeof( outBufferDesc ) ); 
	outBufferDesc.dwSize = sizeof( outBufferDesc );
	outBufSize = outBufferDesc.dwBufferBytes = soundChunkSize * 4;
	outBufferDesc.dwFlags = DSBCAPS_CTRLVOLUME;
	outBufferDesc.lpwfxFormat = &playbackFormat;
	result = outDevice->CreateSoundBuffer( &outBufferDesc, &outBuf, NULL );
	DSErrorCheck( result );

	// Create the input buffer
	OutputDebugString("Creating CaptureBuffer\n");
	memset( &inBufferDesc, 0, sizeof( inBufferDesc ) ); 
	inBufferDesc.dwSize = sizeof( inBufferDesc );
	inBufferDesc.dwBufferBytes = soundChunkSize * 2;
	inBufferDesc.lpwfxFormat = audioFormat;
	result = inDevice->CreateCaptureBuffer( &inBufferDesc, &inBuf, NULL );
	DSErrorCheck( result );

	// Get the notification interfaces for the input buffer
	OutputDebugString("Getting notification interface for input buffer\n");
	result = inBuf->QueryInterface( IID_IDirectSoundNotify, (void**)&inNotify );
	DSErrorCheck( result );


	// Start the main sampling loop
	runFlag = TRUE;
	threadHandle = (HANDLE)_beginthread( TalkIOloop, 0, NULL );
	ShiAssert( threadHandle );

	// Set our execution thread to high priority
	SetThreadPriority( threadHandle, THREAD_PRIORITY_TIME_CRITICAL );

	// Begin in listen mode
	CurrentState = Receiving;
	transmissionCount = 0;
	#endif
}


void	CleanupTalkIO( void )
{

	#ifdef CHAT_USED
	HRESULT result;


	// Shut down the sampling thread
	runFlag = FALSE;
	SetEvent( wakeEvent );
	WaitForSingleObject( threadHandle, INFINITE );

	// Free the recording buffer notification interface
	OutputDebugString("Destroying CaptureBuffer notification object");
	result = inNotify->Release();

	// Free the recording buffer
	OutputDebugString("Destroying CaptureBuffer\n");
	result = inBuf->Release();

	// Free the playback buffer
	OutputDebugString("Destroying DSound secondary output buffer\n");
	result = outBuf->Release();

	// Free our compressor input buffer
	delete [] compressorInput;
	compressorInput = NULL;

	// Shut down our comm channel
	CleanupTransport();
	#endif
}


void __cdecl TalkIOloop( void *args )
{
	HRESULT		result;
	void		*data1, *data2;
	DWORD		data1Size, data2Size;
	DWORD		playTime;
	WORD		*src16;
	WORD		*dest16;
	DWORD 		i;
	DWORD		status;
	DWORD		playpos;
	DWORD		writepos;
	char		buf[256];
	DWORD		writeDelta;
	BOOL 		wrapFlag;

	firstPacketDelayed = FALSE;

	while (runFlag) {

		if ( CurrentState == Receiving ) {

			// Wait for a packet to arrive
			if ( DataReady() && firstPacketDelayed ) {

				// Read a packet
				OutputDebugString("Getting a packet\n");
				compressorInputSize = Receive( compressorInput, soundChunkSize*2 );
				ShiAssert( compressorInputSize );

				outBuf->GetCurrentPosition( &playpos, &writepos );
				sprintf( buf, "play pos = %d, writepos = %d\n", playpos, writepos );
				OutputDebugString( buf );

				// get approx # of milliseconds we're ahead of the write
				// pointer
				if ( writePosition < writepos )
				{
					// if we haven't wrapped around the buffer, we haven't
					// kept up with the play.  Set write position to where
					// it's safe to write
						writeDelta = outBufSize - writepos + writePosition;
						/*
					if ( wrapFlag )
					{
						writeDelta = outBufSize - writepos + writePosition;
					}
					else
					{
						writePosition = writepos;
						writeDelta = 0;
					}
					*/
				}
				else
				{
					writeDelta = writePosition - writepos;
				}

				// Lock the target buffer
				result = outBuf->Lock( writePosition, compressorInputSize,
									   &data1, &data1Size, &data2, &data2Size, 
									   0 );
				DSErrorCheck( result );

				sprintf( buf, "Writing to = %d, bytes = %d\n", writePosition, compressorInputSize );
				OutputDebugString( buf );

				writeDelta = 1000 * writeDelta / playbackFormat.nAvgBytesPerSec;

				// Put the data into the output buffer
				memcpy( data1, compressorInput, data1Size );
				if (data2) {
					memcpy( data2, &compressorInput[data1Size], data2Size );
					writePosition = data2Size;
					wrapFlag = TRUE;
				} 
				else
				{
					writePosition += data1Size;
					wrapFlag = FALSE;
				}

				if ( writePosition == outBufSize)
				{
					writePosition = 0;
					wrapFlag = TRUE;
				}
				
				// Unlock the target buffer
				result = outBuf->Unlock( data1, data1Size, data2, data2Size );
				DSErrorCheck( result );

				// Play the sound
				// result = outBuf->Play( 0, 0, DSBPLAY_LOOPING );
				// DSErrorCheck( result );
				result = outBuf->GetStatus( &status );
				if ( !(status & DSBSTATUS_PLAYING) )
				{
					result = outBuf->Play( 0, 0, DSBPLAY_LOOPING );
					DSErrorCheck( result );
				}

				// Sleep for a while --- about 1/2 the play time
				playTime = 1000 * compressorInputSize / playbackFormat.nAvgBytesPerSec;

				// insure we don't get too far ahead of the write delta
				if ( writeDelta > playTime )
					playTime = writeDelta;

				Sleep( playTime/2 );

				// try our hardest to get the next packet -- up until
				// the point the write delat is too small
				while ( !DataReady() )
				{
					outBuf->GetCurrentPosition( &playpos, &writepos );
					if ( writePosition < writepos )
					{
						writeDelta = outBufSize - writepos + writePosition;
					}
					else
					{
						writeDelta = writePosition - writepos;
					}
					writeDelta = 1000 * writeDelta / playbackFormat.nAvgBytesPerSec;
					if ( writeDelta < 300 )
					    break;
				}
			}
			else
			{
				// 1st packet will have a slight delay
				if ( DataReady() )
				{
					firstPacketDelayed = TRUE;
					// OutputDebugString("Setting Packet delay\n");
				}
				else
				{
					firstPacketDelayed = FALSE;
					// OutputDebugString("No Packet delay\n");
				}

				result = outBuf->GetStatus( &status );
				if ( status & DSBSTATUS_PLAYING )
				{
					result = outBuf->Stop();
					result = outBuf->SetCurrentPosition( 0 );
					DSErrorCheck( result );
					writePosition = 0;
				}
				wrapFlag = FALSE;
				Sleep( 100 );

			}

		} else {
			// Wait for signal to wake up
			WaitForSingleObject( wakeEvent, INFINITE );

			// Lock the DSound buffer
			result = inBuf->Lock( readPosition, soundChunkSize, &data1, &data1Size, &data2, &data2Size, 0 );
			DSErrorCheck( result );

			// Copy out any available DSound data
			// memcpy( compressorInput, data1, data1Size );
			dest16 = (WORD *)compressorInput;
			src16 = (WORD *)data1;
			for( i = 0; i < data1Size; i += (4 * DOWNSAMPLE_AMT) )
			{
				*dest16++ = (*src16);
				src16 += (2 * DOWNSAMPLE_AMT);
			}

			if (data2) {
				src16 = (WORD *)data2;
				for( i = 0; i < data2Size; i += (4 * DOWNSAMPLE_AMT) )
				{
   				*dest16++ = (*src16);
					src16 += (2 * DOWNSAMPLE_AMT);
				}
				compressorInputSize = ((BYTE *)dest16) - compressorInput;
				readPosition = data2Size;
			} else {
				compressorInputSize = ((BYTE *)dest16) - compressorInput;
				readPosition += data1Size;
			}

			// Unlock the DSound buffer
			result = inBuf->Unlock( data1, data1Size, data2, data2Size );
			DSErrorCheck( result );

			// Send a packet
			OutputDebugString("Sending a packet\n");
			Send( compressorInput, compressorInputSize );
			packetCount += 1;

#ifdef FLUSH_LOCAL_TRANSMISSIONS
			// Flush the incomming message queue
			while (DataReady()) {
				OutputDebugString("Receiving one to flush\n");
				compressorInputSize = Receive( compressorInput, soundChunkSize*2 );
			}
#endif

		}
	}

	OutputDebugString("Service loop falling off\n");
	result = outBuf->Stop();
}


void BeginTransmission( void )
{
	#ifdef CHAT_USED
	HRESULT				result;

	if (CurrentState == Transmitting) {
		OutputDebugString("Already transmitting!");
		return;
	}

	// Reset our read position
	#ifdef LOOPBACK_TEST
	ResetTransport();
	#endif
	readPosition = 0;
	transmissionCount += 1;
	packetCount = 1;

	// Set our first notification point
	OutputDebugString("Set notifications\n");
	positions[0].dwOffset		= soundChunkSize-1;
	positions[0].hEventNotify	= wakeEvent;
	positions[1].dwOffset		= soundChunkSize + soundChunkSize-1;
	positions[1].hEventNotify	= wakeEvent;
	positions[2].dwOffset		= DSBPN_OFFSETSTOP;
	positions[2].hEventNotify	= wakeEvent;
	result = inNotify->SetNotificationPositions( 3, positions );
	DSErrorCheck( result );

	// Start capturing
	OutputDebugString("Mic is live.\n");
	result = inBuf->Start( DSCBSTART_LOOPING );
	DSErrorCheck( result );

	CurrentState = Transmitting;

	result = outBuf->Stop();
	#endif
}



void EndTransmission( void )
{
	#ifdef CHAT_USED
	HRESULT result;


	if (CurrentState == Receiving) {
		OutputDebugString("Already listening!");
		return;
	}

	// Note that we're stopping our transmission
	CurrentState = Receiving;

	// Stop the microphone sampling
	OutputDebugString("Mic is dead.\n");
	result = inBuf->Stop();
	DSErrorCheck( result );

#ifdef FLUSH_LOCAL_TRANSMISSIONS
	// Flush the incomming message queue
	OutputDebugString("Flushing incomming messages.\n");
	while (DataReady()) {
		OutputDebugString("Receiving one to flush\n");
		compressorInputSize = Receive( compressorInput, soundChunkSize*2 );
	}
#endif

	#endif

}


BOOL
DSErrorCheck( HRESULT result )
{

#ifdef NDEBUG
	switch ( result ) {

      case DS_OK:                       
        return TRUE;
      case DSERR_ALLOCATED:
        MessageBox( NULL, "DSERR_ALLOCATED", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_CONTROLUNAVAIL:
        MessageBox( NULL, "DSERR_CONTROLUNAVAIL", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_INVALIDPARAM:
        MessageBox( NULL, "DSERR_INVALIDPARAM", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_INVALIDCALL:
        MessageBox( NULL, "DSERR_INVALIDCALL", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_GENERIC:
        MessageBox( NULL, "DSERR_GENERIC", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_PRIOLEVELNEEDED:
        MessageBox( NULL, "DSERR_PRIOLEVELNEEDED", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_OUTOFMEMORY:
        MessageBox( NULL, "DSERR_OUTOFMEMORY", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_BADFORMAT:
        MessageBox( NULL, "DSERR_BADFORMAT", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_UNSUPPORTED:
        MessageBox( NULL, "DSERR_UNSUPPORTED", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_NODRIVER:
        MessageBox( NULL, "DSERR_NODRIVER", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_ALREADYINITIALIZED:
        MessageBox( NULL, "DSERR_ALREADYINITIALIZED", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_NOAGGREGATION:
        MessageBox( NULL, "DSERR_NOAGGREGATION", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_BUFFERLOST:
        MessageBox( NULL, "DSERR_BUFFERLOST", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_OTHERAPPHASPRIO:
        MessageBox( NULL, "DSERR_OTHERAPPHASPRIO", "DSound Error", MB_OK );
        return FALSE;
      case DSERR_UNINITIALIZED:
        MessageBox( NULL, "DSERR_UNINITIALIZED", "DSound Error", MB_OK );
        return FALSE;
	  case E_NOINTERFACE:
		MessageBox( NULL, "E_NOINTERFACE", "DSound Error", MB_OK );
        return FALSE;
      default:
		MessageBox( NULL, "UNKNOWN ERROR CODE", "DSound Error", MB_OK );
		return FALSE;
    }

#else // NDEBUG
	if ( result == DS_OK )
		return TRUE;

	return FALSE;
#endif
}

#endif