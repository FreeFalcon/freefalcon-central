#ifndef	_FALC_VOICE_H_
#define _FALC_VOICE_H_

#include "f4thread.h"

/* CONSTANTS */
enum{
	MAX_VOICE_BUFFERS	=	2,
	MAX_BUFFER_NUM		=	2,
	DSB_SIZE			=	0x1060,
	SILENCE_KEY			=	0x00, //0x80
	BUFFER_NOT_IN_QUEUE	=	0,
	BUFFER_IN_QUEUE		=	1,
	BUFFER_FILLED		=	2,
};

typedef struct {
	int                 mesgNum;			
	int                 status;				// buffer filled VM sets to TRUE, VC sets to false in fillBuffer
	unsigned char       *waveBuffer;        // audio buffer
	DWORD               dataInWaveBuffer;   // audio data to process
	DWORD               waveBufferLen;      // length of audio buffer
	DWORD               waveBufferWrite;    // write offset
	DWORD               waveBufferRead;     // read offset
	F4CSECTIONHANDLE*    criticalSection;	// Thread critical section information
} VOICE_STREAM_BUFFER;

typedef struct {
	int                 status;					// voice status
	int					streamBuffer;			// buffer to stream
	WAVEFORMATEX        waveFormat;				// Wave header information
	int                 voiceHandle;			// audio handle
//	int					currConv				// current conv playing
//	int					convQCount;				// number of conversations pending
} VOICE;

typedef struct {
//	FILE			*lhCompFile;
	long			bytesDecoded;		/* read from compressed file */
	long			bytesRead;			/* read from compressed file */
	long			fileLength;			/* uncompressed file size */
	long			compFileLength;     /* compressed file size */
	char			*dataPtr;			// memory location of compressed data
	} COMPRESSION_DATA;

class FalcVoice
	{
	public:
	VOICE				*voiceStruct;
	VOICE_STREAM_BUFFER voiceBuffers[MAX_VOICE_BUFFERS];
	int					FalcVoiceHandle;
	long				silenceWritten;
	int					channel;
	COMPRESSION_DATA	*voiceCompInfo;
	int					exitChannel;

	FalcVoice( void );
	~FalcVoice( void );

	void CreateVoice( void );
	void InitCompressionData( void );
	void PlayVoices( void );
	void SilenceVoices( void );
	void UnsilenceVoices( int SoundGroup );
	void FVResumeVoiceStreams( void );
void FalcVoice::SetMesgNum( int bufferNum, int mesgNum );
	void InitializeVoiceBuffers( void );
	void InitializeVoiceStruct( int bufferNum );
	void InitWaveFormatEXData( WAVEFORMATEX	*waveFormat );
	void CleanupVoice( void );
	void SetVoiceChannel( int channelNo );
	void PopVCAddQueue( );
	VOICE_STREAM_BUFFER *GetVoiceBuffer( int bufferNum );
	void AllocateCompInfo( void );
	void InitCompressionFile( void );
	void CleanupCompressionBuffer( void );
	void BufferManager( int buffer );
	void BufferEmpty( int buffer );
	void ResetBufferStatus( void );

	void DebugStatus( void );
	};
 // MLR 1/29/2004 - added protos
void SetVoiceVolumes(void);
void SetVoiceVolume(int channel);



#endif