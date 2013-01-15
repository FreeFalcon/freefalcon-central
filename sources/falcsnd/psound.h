#ifndef _SOUND_LIB_
#define _SOUND_LIB_

#ifndef _INC_STDIO
#include <stdio.h>
#endif

#ifndef __DSOUND_INCLUDED__
#include "dsound.h"
#endif

#include "grtypes.h"
enum
{
	SND_NO_HANDLE       =0,

	    // JPO - best guess at what the next 4 mean
	    SND_BIT1	    =0x00000001, // no idea what this is - its used but never tested that I can find
	SND_STREAM_PART2    =0x00000002, // were processing 2nd part of the buffer
	SND_STREAM_DONE	    =0x00000004, // weve read the whole stream now
	SND_STREAM_FINAL    =0x00000008, // we've played the whole stream now

	// Use SFX_ flags instead of these!
	//SND_LOOP_SAMPLE     =0x00000004, // MLR was: DSBSTATUS_LOOPING, same value. // MLR 12/6/2003 - OBSOLETE // Use SFX_ flags 
	//SND_USE_3D          =0x00000100, // MLR 12/6/2003 - - OBSOLETE
	//SND_OVERRIDE        =0x00000400, // MLR 12/6/2003 - - OBSOLETE
	//SND_EXCLUSIVE       =0x00000800, // MLR 12/6/2003 - - OBSOLETE

	SND_STREAM_FILE     =0x00001000,
	SND_STREAM_MEMORY   =0x00002000,
	SND_STREAM_CALLBACK =0x00004000,
	SND_STREAM_LOOP     =0x00008000,
	SND_STREAM_FADE_OUT =0x00010000,
	SND_STREAM_FADE_IN  =0x00020000,
	SND_STREAM_CONTINUE =0x00040000,
	SND_STREAM_FADEDOUT =0x00080000,
	SND_STREAM_PAN_RT   =0x00100000,
	SND_STREAM_PAN_LT   =0x00200000,
	SND_STREAM_PAN_CIR  =0x00400000,
	SND_IS_IMAADPCM		=0x10000000,
	SND_USE_THREAD      =0x40000000,

	SND_MIN_VOLUME		=-3000,
	SND_MAX_VOLUME		=0,

	SND_ADPCM_SBLOCK_ALIGN 		=1024,	//ima adpcm block alignment for 16-bit stereo
	SND_PCM_SBLOCK_ALIGN		=4,		//pcm block alignment for 16-bit stereo
	SND_ADPCM_MBLOCK_ALIGN 		=512,	//ima adpcm block alignment for 16-bit mono
	SND_PCM_MBLOCK_ALIGN		=2,		//pcm block alignment for 16-bit mono
	SND_BLOCK_SAMPLE			=1017,	//#samples per block of ima adpcm
	SND_COMPRESSION_RATIO		=4,
	SND_WAV_SCHAN				=2,
	SND_WAV_MCHAN				=1,
};

enum // Stream Callback Messages
{
	SND_MSG_JUST_HAVING_FUN=0,
	SND_MSG_START_FADE,
	SND_MSG_FADE_IN_DONE,
	SND_MSG_FADE_OUT_DONE,
	SND_MSG_STREAM_DONE,
	SND_MSG_STREAM_EOF,
};

class CSoundMgr;
extern CSoundMgr *gSoundDriver;
struct Tpoint;
struct Trotation;
struct SfxDef;

typedef class SoundList  SOUNDLIST;
typedef class SoundStream SOUNDSTREAM;

#define DS3DBUFFERMAX 1

struct slChannel
{
	IDirectSoundBuffer *DSoundBuffer;
	LPDIRECTSOUND3DBUFFER DSound3dBuffer;
	float x,y,z; // MLR 2003-10-17
	float vx,vy,vz;
	float distsq; // set to -1 if unused
	float pitch;
	float vol;
	int uid;
	int Timer;
	int Is3d;
}; //

class SoundList
{
public:
    SoundList();
    SoundList(SoundList *copy, IDirectSound *DSound);
    ~SoundList();
	long ID;
	long Volume;
	long Frequency;
	long Direction;
	long Flags; // Use SFX_ flags 
	float MaxDist,MinDist;
	//IDirectSoundBuffer *DSoundBuffer;

	int DS3DBufferCount;  // always use this and not DS3DBUFFERMAX;
	int Cur3dBuffer; // buffer to assign sound to when the Sample is non-looping.
	int is3d;

	struct SfxDef *Sfx; // for debugging purposes

	struct slChannel Buf[DS3DBUFFERMAX]; // Sample Queue
	SOUNDLIST *Next;
};




typedef struct
{
	short	iSamp0;
	char	bStepTableIndex;
	char	bReserved;
} IMA_BLOCK;

typedef struct
{
	char *src; // source buffer ptr (can point to memory other than srcbuffer
	char *srcbuffer; // Actual malloc'd buffer (filled 1/2 at a time (except for initial read)
	long  bufsize; // size of srcbuffer
	long  sidx; // location we are decoding from
	long  slen; // size of total IMA compressed data
	long  sreadidx; // location we are filling from (reading into from a file)
	long  srcsize; // size of buffer we are streaming from (may be different from bufsize)
	long  didx; // # bytes sent to output
	long  dlen; // total size of output... when didx >= dlen, we are done
	short type; // 1=Mono/2=Stereo
	short Status;
	// Decoding stuff
	long  blockLength; // Block length of section we are decompressing
	short count; // a counter which goes 8 -> 0 and is used for decoding
	// Mono stuff
	short predSampleL;
	long  leftSamples;
	short stepIndexL;
	// Stereo stuff
	short predSampleR;
	long  rightSamples;
	short stepIndexR;
} IMA_STREAM;

typedef struct
{
	char *data;				// actual file data (All except for 1st 8 bytes,free this)
	WAVEFORMATEX *Format;	// ptr to format info in data
	long NumSamples;		// for IMA_ADPCM, (and maybe others) get # samples so we know when we're done
	char *Start;			// ptr to start of sample in data
	long SampleLen;			// Length of sample
} RIFF_FILE;

//JPO turn into a class so we can construct/destruct
// lets use the power of C++
class SoundStream
{
public:
    SoundStream(); 
    ~SoundStream();
	long ID;
	long Volume;
	long CurFade;
	long FadeIn;
	long FadeOut;
	long Frequency;
	long Direction;
	long BytesPerSecond;
	long Status;
	short LoopCount;
	HANDLE fp;
	DWORD Size;
	DWORD HalfSize;
	DWORD StreamSize;
	DWORD OriginalSize;
	DWORD BytesProcessed;
	DWORD LastPos;
	DWORD LoopOffset;
	long HeaderOffset;
	IMA_STREAM *ImaInfo; // Contains ALL info for streaming using IMA_ADPCM (including buffers)
	void *startptr;
	void *memptr;
	DWORD (*Callback)(void *me,char *mem,DWORD Len);
	void (*StreamMessage)(SOUNDSTREAM *me,int Message);
	void *me; // pointer to YOUR CLASS
	IDirectSoundBuffer *DSoundBuffer;
	LPDIRECTSOUNDNOTIFY lpDsNotify; // JPO - notification interface
	HANDLE notif; // notification event
	SOUNDSTREAM *Next;
};

class CSoundMgr
{
	friend class mlrVoice; // MLR
private:
	long MasterVolume;
	long TotalSamples; // Counter which is used for the Sample ID
	long TotalStreams; // Counter used for Stream ID
	BOOL StreamRunning;
	IDirectSound *DSound;
	IDirectSoundBuffer *Primary;
	LPDIRECTSOUND3DLISTENER Ds3dListener;
	SOUNDLIST *SampleList;
	SOUNDLIST *DuplicateList;
	SOUNDSTREAM *StreamList;
	HANDLE signalEvent; // event to signal to the thread that somethings changed
	BOOL use3d; // use 3d stuff or not.

	// this is for chat buffer stuff....
	// LPDIRECTSOUNDCAPTURE		chatInputDevice;

public:
	CSoundMgr();
	~CSoundMgr();
// Installation Functions
	BOOL InstallDSound(HWND hwnd,DWORD Priority,WAVEFORMATEX *fmt);
	void RemoveDSound();
	long SetMasterVolume(long NewVolume);
	long GetMasterVolume();
// Added samples to Manager
	long LoadWaveFile(char *Filename,long Flags, struct SfxDef *sfx);
	LPDIRECTSOUNDBUFFER LoadWaveFile(char *Filename, struct SfxDef *sfx);
	long AddRawSample(WAVEFORMATEX *Header,char *Data, long size,long Flags);
// Removing samples from manager
	void RemoveSample(long ID);
	void RemoveDuplicateSample(long ID);
	void RemoveAllSamples();
// Playing samples
	BOOL PlaySample(long ID,long Flags);
// Stopping samples
	BOOL StopSample(long ID);
	BOOL StopAllSamples (void);
// Modifying sample playback
	BOOL SetSamplePitch(long ID, float NewPitch);
	BOOL SetSampleVolume(long ID, long Volume);
	BOOL SetSamplePan(long ID, long Direction);
	// 3d effects for samples
	BOOL SetSamplePosition(long ID, float x, float y, float z, float pitch, float vol, float vx, float vy, float vz, float dist, int uid, int is3d);
	BOOL Disable3dSample(long ID);
	void AssignSamples(void);

// Querying a Sample
	int GetSampleVolume(long ID);
	BOOL IsSamplePlaying(long ID, int UID);
	DWORD SampleStatus(SOUNDLIST *Sample);
// Adding a Stream Stream
	long CreateStream(WAVEFORMATEX *Format,float StreamSeconds); // Quesize is in seconds
	void SetMessageCallback(int ID,void (*cb)(SOUNDSTREAM *,int));
	void RemoveStream(long ID);
	void RemoveAllStreams();
	void ResumeAllStreams();
// Starting a Stream
	void SilenceStream(SOUNDSTREAM *Stream,DWORD Buffer,DWORD Length);
	BOOL StartFileStream(long StreamID,char *filename,long Flags,long offset=0);
	BOOL StartMemoryStream(long StreamID,char *Data,long size);
	BOOL StartMemoryStream(long StreamID,RIFF_FILE *file,long Flags);
	BOOL StartCallbackStream(long StreamID,void *classptr,DWORD (*cb)(void *me,char *mem,DWORD Len));
	long SetStreamVolume(long ID, long Volume);
	void ResumeStream(long ID);
	void ResumeStreamFadeIn(long ID);
// Stopping a Stream
	void StopStream(long StreamID);
	void StopStreamWithFade(long StreamID);
	void StopAllStreams();
	void PauseStream(long StreamID);
	void FadeOutStream(long StreamID);
// Querying a Stream
	long GetStreamPlayTime(long ID);
	BOOL IsStreamPlaying(long ID);
	DWORD StreamStatus(SOUNDSTREAM *Stream);
// Setting Stream options
	void SetFadeIn(long ID,long FadeVolume);
	void SetFadeOut(long ID,long FadeVolume);
	void SetLoopCounter(long ID,long count);
	void SetLoopOffset(long ID,DWORD offset);
// Function Only for Use of Mixer
	SOUNDSTREAM *FirstStream();
	void StreamStop(SOUNDSTREAM *Stream);
	void StreamStopWithFade(SOUNDSTREAM *Stream);
	void StreamPause(SOUNDSTREAM *Stream);
	void StreamFadeOut(SOUNDSTREAM *Stream);
	void StreamResume(SOUNDSTREAM *Stream);
	void StreamResumeFadeIn(SOUNDSTREAM *Stream);
// Function to pass data into the Stream buffer
	DWORD ReadStream(SOUNDSTREAM *Stream,DWORD Buffer,DWORD Length);
	void RestartStream(SOUNDSTREAM *Stream);
	SOUNDSTREAM * FirstStreamBuffer();

	// 3d stuff
	void SetCameraPostion(Tpoint *camPos, Trotation *camRot, Tpoint *camvel, bool Reset);
	struct
	{
		float x,y,z;
	} CamPos, CamVelocity;

// Chat IO stuff
	BOOL ChatSetup( void );
	void ChatCleanup( void );
	void ChatToggleXmitReceive( void );
	SOUNDLIST *FindSample(long ID);
	SOUNDSTREAM * FindStream(long ID);
	long SkipRiffHeader(FILE *fp);
	long SkipRiffHeader(HANDLE fp);
	long LoadRiffFormat(char *filename,WAVEFORMATEX *Format,long *HeaderSize,long *SampleCount);
	long LoadRiffFormat(HANDLE fp,WAVEFORMATEX *Format,long *HeaderSize,long *SampleCount);
	long FillRiffInfo(char *memory,RIFF_FILE *riff);
	RIFF_FILE *LoadRiff(char *filename);
// IMA ADPCM decompression stuff
	long StreamIMAADPCM(SOUNDSTREAM *Stream,char *dest,long dlen); // This functions reads from a file, & calls appropriate decode
	long MemStreamIMAADPCM(SOUNDSTREAM *Stream,char *dest,long dlen); // This functions reads from memory, & calls appropriate decode
	static void DSoundCheck(HRESULT hr);

private:
	long ConvertVolumeToDB(long Percentage);
	long ConvertPanToDB(long Direction);
	long AddSampleToMgr(long Volume,long Frequency,long Direction,IDirectSoundBuffer *NewSound,long Flags, SfxDef *sfx);
	long AddStreamToMgr(long Volume,WAVEFORMATEX *Header,long StreamSize,IDirectSoundBuffer *NewSound);
	SOUNDLIST *AddDuplicateSample(SOUNDLIST *Sample);
	static unsigned int __stdcall StreamThread(void *Param);
	void ThreadHandler();
// Thread function to handle streaming from disk/memory
	long StreamImaS16(IMA_STREAM *Info, char *dBuff, long dlen);
	long StreamImaM16(IMA_STREAM *Info, char *dBuff, long dlen);
	long ImaDecodeS16(char *sBuff, char *dBuff,	long bufferLength);
	long ImaDecodeM16(char *sBuff, char	*dBuff,	long bufferLength);
	BOOL BuildObjectList(HANDLE hArray[], int *nHandles, SoundStream *streams[]); // work out whats happening
	void ProcessStream(SoundStream *stream);
	void NotifyThread() { SetEvent(signalEvent); }; // poke the thread
	void SetNotification(SOUNDSTREAM *Stream); // work out notification positions
	short IMA_SampleDecode(short nEncodedSample,short nPredictedSample, short nStepSize);
	short IMA_NextStepIndex(short nEncodedSample, short nStepIndex);
	BOOL IMA_ValidStepIndex(short nStepIndex);
};



#endif
