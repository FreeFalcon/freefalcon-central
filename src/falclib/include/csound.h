#ifndef CSOUND_H
#define CSOUND_H

// csound.h
// sound FX and mixer system class definitions
// Chuck Hughes, 1995
// Copyright Spectrum Holobyte


// must pre-include windows and mmsystem.h

// Constant IDs returned by class IsA() functions
#include "IsA.h"    


#define STARTINGSOUNDREQUESTS	16	
#define STARTINGTRACKS			12


// 10 seconds Raw B stereo, 20 seconds mono (22050 samp/sec x 2 bytes/samp)
#define MIXINGBUFFERINT32S		262144
// DO NOT define MIXINGBUFFERBYTES AS MIXINGBUFFERINT32S*4, it causes MSC to barf, 
// just some of the time, with bad results, not exceptions
#define MIXINGBUFFERBYTES		1048576

#define MONO 1
#define STEREO 2
#define RATE22K 22050
#define SAMPLESIZE8BITS 8
#define SAMPLESIZE16BITS 16

#define MONO16BIT22K 16221

#define MAXVOLUME 63
#define ZEROVOLUME 0
#define HALFVOLUME 31
#define DEFAULTVOLUME MAXVOLUME
#define WAVEOUTMAXVOLUME 0xFFFFFFFF
#define WAVEOUT1SIDEMAXVOLUME 0xFFFF


// amount of bytes ahead of the currently playing point to
// use for margin, to prevent clipping or loss of front of wave
#ifdef _DEBUG
	#define MIXERRORMARGIN 4096
#else
	#define MIXERRORMARGIN 2048
#endif

// milliseconds between allowed mixer buffer refills & cleanup
#define MIXMAINTDELAY 250

// underlying sound system in use 
#define USINGWAVEOUT 1
#define USINGDIRECTSOUND 2

// using time from clock to determine position, 
// or depending on reports from waveout
#define USINGTIMEPOSITION 1
#define USINGWAVEOUTPOSITION 2

#define DOSCLOCKRESOLUTION 55

// number of maintenance segments the mixing buffer is defined as holding
// must hold bytes divisible by 8 each segment.
#define NUMSEGS 8
 

#define STARTSIDERANGE  7003
#define LEFTSIDE		STARTSIDERANGE
#define RIGHTSIDE		STARTSIDERANGE +1
#define BOTHSIDES		STARTSIDERANGE +2
#define EITHERSIDE		STARTSIDERANGE +3


#define REQUESTDENYNOCHANNEL	5004
#define REQUESTDENYNOSOUNDCARD	5005

#define REQUESTOK				1
#define JUSTNOGOOD				-1

#define STARTINGIDNUM 10000
#define IDHANDLE int


// structs & unions to aid decoding of stereo 16 bit (32 bit int ) packets
struct TwoShorts{
	short SideA;
	short SideB;
};

union Stereo4{
    unsigned int packet;
	TwoShorts LR;
};


	 
// forward declares and externs
class CSoundRequest;
class CWaveRequest;
class CSoundManager;
class CWaveTrack;

// use IDGenerator if you want unique serial control or object ID's
// these start above the usual range for dialogs, etc, but can be used for 
// control generated on the fly, homegrown controls, object IDs, and so on.
IDHANDLE IDGenerator();
// various ways of printing debugs, with optional formatted int or uns
void DebugBox(char * title, char * message, unsigned int value);
void DebugBox(char * title, char * message, int value);
void DebugBox(char * message);
void DebugBox(char * title,char * message);

// threading support, optional threads
UINT SoundThread(LPVOID pParam);
void KillSoundThread(CSoundManager * pSoundManager);

class CRaw{
	friend class CWaveTrack;
	int mSoundType;
	int mRequestedBufferLengthBytes;
	int mBufferLengthBytes;
	int * mpBuffer;
	BOOL mIsValid;
public:
	BOOL IsValid(void){
		return mIsValid;
	}
	int GiveSoundType(void){
		return mSoundType;
	}
	int GiveRequestedLength(void){
		return mRequestedBufferLengthBytes;
	}

	int GiveActualLengthBytes(void){
		return mBufferLengthBytes;
	}
	char * GiveCopyOfBufferPointer(void){
		return (char *)mpBuffer;
	}
	CRaw(int soundtype,int lengthbytes){
		mIsValid = FALSE;
		mpBuffer = NULL;
		mBufferLengthBytes = 0;
		mSoundType = JUSTNOGOOD;

		if (soundtype != MONO16BIT22K){
			return;
		}
		mSoundType = soundtype;
		mRequestedBufferLengthBytes = lengthbytes - (lengthbytes%4);
		if (!mRequestedBufferLengthBytes){
			return;
		}
		mIsValid = TRUE;
	}
    CRaw(void){
        mIsValid = FALSE;
    }
};

// CMSFrameCode class gives time in 30 fps non-drop format, plus an
// optional millsecond remainder. Thus a variety of frame-synchronized and
// unsynchronized operations can be supported.
class CMSFrameCode{
	int mHour;
	int mMinute;
	int mSecond;
	int mFrame;
	double mMSRemainder;
public:
	// given even SMPTE time constructor
	CMSFrameCode(int hour,int minute,int second,int frame){
		mHour = hour;
		mMinute = minute;
		mSecond = second;
		mFrame = frame;
		mMSRemainder = 0;
	}
	// make SMPTE + float remainder given milliseconds
	CMSFrameCode(int milliseconds){
		int remaining = 0;
		mHour = milliseconds / 3600000;
		remaining = milliseconds % 3600000;
		mMinute = remaining / 60000;
		remaining = remaining % 60000;
		mSecond = remaining / 1000;
		remaining = remaining % 1000;
		mFrame = ((int) ((double)remaining / (33.0 +1.0/3.0)) );
		mMSRemainder = fmod((double)remaining,(33.0+1.0/3.0));
	}
	CMSFrameCode(){
		mHour = 0;
		mMinute = 0;
		mSecond = 0;
		mFrame = 0;
		mMSRemainder = 0.0;
	}
	CMSFrameCode& operator=(CMSFrameCode& time){
		mHour   = time.mHour;
		mMinute = time.mMinute;
		mSecond = time.mSecond;
		mFrame  = time.mFrame;
		mMSRemainder = time.mMSRemainder;
		return *this;
	}
	CMSFrameCode& operator+=(CMSFrameCode& timelength){
		int milliseconds = GiveMilliseconds();
		unsigned int otherms = timelength.GiveMilliseconds();
		milliseconds += otherms;
 		int remaining = 0;
		mHour = milliseconds / 3600000;
		remaining = milliseconds % 3600000;
		mMinute = remaining / 60000;
		remaining = remaining % 60000;
		mSecond = remaining / 1000;
		remaining = remaining % 1000;
		mFrame = ((int) ((double)remaining / (33.0 +1.0/3.0)) );
		mMSRemainder = fmod((double)remaining,(33.0+1.0/3.0));
		return *this;
	}
    inline unsigned GiveMilliseconds(){
        double milliseconds = mHour * 3600000 +
		                mMinute * 60000 +
		                mSecond * 1000 +
		                mFrame * (33.0 +1.0/3.0) +
		                mMSRemainder + 0.00000001; // just in case of stupid float packages
        return (unsigned) milliseconds;
    }
	inline unsigned GiveHours(void){return mHour;}
	inline unsigned GiveMinutes(void){return mMinute;}
	inline unsigned GiveSeconds(void){return mSecond;}
 	inline unsigned GiveFrames(void){return mFrame;}
 	inline double 	GiveMillisecondRemainder(void){return mMSRemainder;}
    inline unsigned TimeToBytes(double bytespersecond){
        return (unsigned) ((((double)GiveMilliseconds()) * bytespersecond)/1000.0);
    }
};

// cSoundBufSegment class is for breaking down the mixing buffer
// into sections that are simultaneously playing, being mixed, being cleaned,
// with the current segment being locked away from the 
// cleanup routines.
class CSoundBufSegment{
	friend class CSoundManager;
    BOOL mCleanFlag; // has been cleared flag
    int mBufferOffset;      // offset into main buffer
    int mSegmentLength;     // bytes in segment
	CSoundManager * pSndMgr;
    BOOL mIsValid;
public:
    virtual BOOL IsValid(void){
        return mIsValid;
    }
    virtual int IsA(void){
        return  BUFSEGMENT;
    }
    void OnSBSInit(void);
    CSoundBufSegment(int segoffset, int seglength, CSoundManager * SndMgr);
    CSoundBufSegment(void){
        mIsValid = FALSE;
    }
    void Execute();
    void CleanSeg();
	BOOL SegmentBeingPlayed();
    inline BOOL IsClean(){
            return mCleanFlag;
    }
};


// CWaveTrack  class for digital wave tracks internal to the sound manager
class CWaveTrack:public CBaseObject{
private:
	friend class CSoundManager;
	friend class CWaveRequest;
	char * mFileName;
	long mFileSize;
	HMMIO mMMIOfhandle;
	char * mWaveBuffer;
	unsigned int mBufferLength;
	unsigned int mDataLength;
	BOOL mIsLoadedNow;
	int mLinkCount;
	BOOL mIsValid;
	int mMonoOrStereo;
	void ClearTrack(void);
	CRaw * mpRawBufRequest;
protected:
	inline void DecrementLinkCount(void){
		mLinkCount--;
		if(mLinkCount <0)
			mLinkCount = 0;
	}
	inline void IncrementLinkCount(void){
		mLinkCount++;
	}

	inline int GetLinkCount(){
		return mLinkCount;
	}
public:
	virtual BOOL IsValid(void){
		return mIsValid;
	}
   	virtual int IsA(void){ 
		return WAVETRACK;
	}

	void LoadNow(void);
	inline BOOL IsLoadedNow(void){
		return mIsLoadedNow;
	}
	void OnCWTInit();
	CWaveTrack(char * filename);
	CWaveTrack(CRaw * crawreq);
    CWaveTrack(void){
        mIsValid = FALSE;
    }
	virtual ~CWaveTrack(void);
};


// base class for requests of wave or MIDI patch play
class CSoundRequest:public CBaseObject{
private:
	// sound manager controls and manages requests, including deletions
	friend class CSoundManager;
protected:
	BOOL mIsValid;
	char * mFileName;
	int mRequestID;
	int mSpeakerRequested;
	unsigned mVolumeRequested;
	BOOL mLoopingDeclared;
	CMSFrameCode mStartTime;
	CMSFrameCode mLengthInTime;
	BOOL mPlayPending; // has a play comand been made but not completely mixed
public:
	void OnCSRInit(void);
	CSoundRequest(void);
    virtual ~CSoundRequest(void){}
	virtual void SetLengthInTime(CSoundManager * sm) = 0;
	virtual BOOL IsValid(void){
		return mIsValid;
	}
    virtual int IsA(void) =0;

	int RequestSpeaker(int speaker);
	int GiveSpeakerRequested(){
		return mSpeakerRequested;
	}
	BOOL SetAverageVolume(unsigned volume);
	void SetLooping(BOOL state);
	BOOL IsLooping(){
		return mLoopingDeclared;
	}
	void SetStartTime(CMSFrameCode& start){
		mStartTime = start;
	}
	CMSFrameCode& GiveStartTime(void){
		return mStartTime;
	}
	IDHANDLE GiveRequestID(void){
		return mRequestID;
	}
	int IsPlayingNow(CSoundManager * sm);
	int IsPlayOver(CSoundManager * sm);
	CMSFrameCode HowLongUntilFinished(CSoundManager * sm);
	void SetPlayPending(BOOL value){
		mPlayPending = value;
	}
	BOOL PlayPending(){
		return mPlayPending;
	}

};


// class for generating Dopplers, approaching and retreating volume,
// side to side motion, echo, ...
class CSoundFXRequest:public CSoundRequest{
public:
	virtual int IsA(void){ 
		return FXREQUESTTYPE;
	}
    CSoundFXRequest(void){}
    virtual ~CSoundFXRequest(void){}
};


// class for requesting a MIDI patch play, one of the 8 or so
// common sounds we will keep in digital wave patches
class CPatchRequest:public CSoundRequest{
public:
	virtual int IsA(void){ 
		return PATCHREQUESTTYPE;
	}
    CPatchRequest(void){}
    virtual ~CPatchRequest(void){}
};



// User-accessible class for requesting wave play specifically
class CWaveRequest:public CSoundRequest{
private:
	friend class CSoundManager;
	// corresponding track, may be one track for several requests (links)
	CWaveTrack * mpTrack;
	unsigned int mTotalBytesMixedSoFar;	// last byte offset in track that was mixed in
	unsigned int mLocationLastMixedAt;	// mixing buffer offset where the last byte was mixed in
public:
	CWaveRequest(char * SoundName);
	CWaveRequest(CRaw & craw);
    CWaveRequest(void){
        mIsValid = FALSE;
    }
    virtual ~CWaveRequest(void){}
	virtual int IsA(void){ 
		return WAVEREQUESTTYPE;
	}

	CRaw * mCRawBuf;
	void OnWRInit();
	void ResetTiming();
	virtual void SetLengthInTime(CSoundManager * sm);
	BOOL IsLoadedNow();
	inline unsigned GiveBytesMixed(void){
		return mTotalBytesMixedSoFar;
	}
	inline void AddBytesLastMixed(int qtyjustmixed){
		mTotalBytesMixedSoFar += qtyjustmixed;
	}
	void ResetPlayPending(){
		mPlayPending = FALSE;
		mTotalBytesMixedSoFar = 0;
	}
	BOOL AllBytesAreMixed(){
		if(mpTrack){
			if(mTotalBytesMixedSoFar >= mpTrack->mDataLength){
				return TRUE;
			}
		}
		return FALSE;
	}
	unsigned MillisecondsMixedSoFar(void);
};	
 

// This class is the main public interface to the sound
// system, though it's public API's. It is a 
// master mixer, loader, track manager, timing source
// and effects generator.
class CSoundManager{
	int mStereoOrMonoOutput;
	int mSampleRateOutput;
	int mSampleSizeOutputBits;
	int mBlockSizeBytes;
	int mOutputBPS;
	int mMonoTrackBPS;
	int mStereoTracksBPS;

	BOOL mIsValid; // was this object properly instantiated and started up without hitches
	int SetMillisecond_timeGetTime(void);   // request millisecond timer from system
	void ReleaseMillisecond_timeGetTime(void);  // release it as documented in API
	unsigned int mTimerResolution;	// timer resolution actually obtained, hopefully
    unsigned int mTimerType;        // where are we getting the time from this time
    unsigned int mGameStartTime;    // time the SM was constructed, hopefully near program start, MS
	unsigned int mMixingStartTime;	// time that we started mixing data last, milliseconds
	unsigned int StartGameClock(void);	// sets mGameStartTime base at startup, in system milliseconds
	unsigned int StartMixingClock(void);	// sets mMixingStartTime base, used during mixing, various sources
	BOOL mSoundOutMechanismRunning;	// flag whether we are playing
    unsigned int mOutputSystemType; // waveout, directsound, other?
	int OffsetToMixAtNow(void);// for immediate mixing in, use this to figure buffer position
	friend class CWaveTrack;
	friend class CWaveRequest;
	friend class CSoundBufSegment;


	friend UINT SoundThread(LPVOID pParam);
	friend void KillSoundThread(CSoundManager * pSoundManager);
	volatile BOOL mSoundThreadKillFlag; // signal to background mixing thread to die
    volatile BOOL mBlockSoundThread;    // Temporarily block it while array ops, etc
	CBOPArray * aRequests;
	CBOPArray * aTracks;
	char * mpMixingBuffer;	// defined as char * but must be 32 bit aligned
	HWAVEOUT mhWaveOut;
	PWAVEHDR mpOutHeader;
	PMMTIME mpWaveOutCurrentTime;
    Stereo4 * mpWaveOutOverallVolumes;

	BOOL MixWaveRequestImmediate(CWaveRequest * associatedWR); 
	int MixWaveRequestAtOffset(CWaveRequest * associatedWR, int trackLoc, int mixlength, int mixbufloc);
	int Mix16MonoIn16MonoOut(CWaveRequest * associatedWR, int trackloc,int mixlength,int offset);
	int MixStereoOut(CWaveRequest * associatedWR, int trackloc,int mixlength,int offset);

	// For use in ongoing play, mixing at end of an already mixed section of track,
	// check whether it will be in the section currently being mixed 
	// for potential, but not absolute, avoidance.
	BOOL LocIsInCurrentPlayingSegment(int ProspectiveMixingLoc);

    // continue mixing for those imcompletely mixed or looping segments
	int ContinueMixingWaveRequest(CWaveRequest * WRequest);
	inline unsigned OutputFormatBytesToMilliseconds(unsigned bytes){
        return (unsigned) (  ((double)bytes) / ((double)mOutputBPS) * 1000.0 );
    }
	BOOL IsNextMixTimeInCurrentBufferTimeFrame(CWaveRequest * WRequest);

	int FindTrackByPointer(CWaveTrack * Track);
	IncludeWaveRequest(CWaveRequest* candidate);
	int FindRequestByPointer(CSoundRequest * Request);
	int FindRequestByIDNum(int IDNum);
	void ClearMixingBuffer(unsigned int from,unsigned int bytestoclear);
	HWND mhWndMsgWindow;
	CSoundBufSegment * BufSegs[NUMSEGS];
	int GetWaveOutSoundPositionNow(void);
	// current sound buffer play location in bytes
	// based on either wave out bytes loc or millisecond
	// time-dependent calcs, depending on time system selection switch at setup
	int ExactPlayLocationNow(void);

	void OnCSMInit(int monoorstereo);

public:

	// contruct this at the beginning of play, which will start the
	// game clock and set up the buffering. The messages passed to
	// the window handle provided can be ignored.
	CSoundManager(HWND msgwindow, int monoorstereo);
    CSoundManager(void){
        mIsValid = FALSE;
    }
	~CSoundManager(void);
	BOOL IsValid(){
		return mIsValid;
	}


	// call execute in the main application OnIdle loop to give it the 
	// required time shares in between other events, like frames
	void Execute(void);  // our time share, call in on idle and for preparations
	BOOL StartNow(void); // start SM ring buffer system running, clock 00:00:00:00
	void StopNow(void);  // stop ring buffer system running

	// game clock current time in milliseconds
	// gets current milliseconds since StartMyClock()
	unsigned int GameMillisecondTimeNow(void);
	// mixer clock Current time in milliseconds
	// gets current milliseconds since mixer StartNow()
	unsigned int MixerMillisecondTimeNow(void);	
	// give time since game start in smpte30non-drop + remainder
    CMSFrameCode GameSMPTETimeNow();
	// give time since mixer start in smpte30non-drop + remainder
    CMSFrameCode MixerSMPTETimeNow();

	// giving a path, request a wave buffer and ID, which is 
	// an error return if the wave could not be opened.
	IDHANDLE WaveRequest(char * filepath);
	// create a wave request, supply a buffer to be manipulated 
	// by the application programmer
	IDHANDLE WaveRequest(CRaw &);
	// users can remove requests with finiwhen not needed
	// if there is more than one request for a wave, the 
	// track is not removed, but a use count is decremented
 	void Fini(IDHANDLE& RequestID);
	// Deallocate/close all requests at once. Current IDs in your code will have values, 
	// but they will be no good. Used only when quitting (E.G. destructor of CScenario, 
	// level, or entire program. Alternative to individual Fini(ID) of each request.
	void CloseAllRequests(void);

	// play ASAP, for weapons etc.
	IDHANDLE PlayNow(IDHANDLE RequestID);
	// declare looping and start looping ASAP
	IDHANDLE LoopNow(IDHANDLE RequestID);
	// declare looping TRUE, then later any play command will start the process, 
	// FALSE to get default single play again
	IDHANDLE DeclareLooping(IDHANDLE IDNum, BOOL loopstate);
    // ***************************************************************************<<<
    // need to add a DeclareNotLooping(IDHANDLE IDNum); for leon & us
    // also need to unmix a request sooner, for leon, and us
    
    // play ASAP once and discard, recover memory
	IDHANDLE PlayNowAndDiscard(IDHANDLE& IDNum);
	// play some length of time after now
	IDHANDLE PlayAtSMPTETimeOffset(IDHANDLE IDNum, CMSFrameCode waitfor);
	// play at some particular smpte time
	IDHANDLE PlayAtSMPTETime(IDHANDLE IDNum,CMSFrameCode when);

	// request speaker(s) to play a particular request through
	IDHANDLE RequestSpeaker(IDHANDLE IDNum,int speaker);
    // Set overall volume out the speakers
    MMRESULT SetSpeakerVolumes(unsigned short left, unsigned short right);
    // report speaker volumes through function parameters
    void GetSpeakerVolumes(unsigned short & left, unsigned short & right);

	// Set Average mixer volume for a request (like mixer track slider position)
    // from 0 to 63, whole number volume. See const MAXVOLUME, ZEROVOLUME, HALFVOLUME
    // and DEFAULTVOLUME
	IDHANDLE SetRequestVolume(IDHANDLE IDNum, unsigned volume);

	// These next 2 queries return TRUE, FALSE, and JUSTNOGOOD, check with ==
	// JUSTNOGOOD usually means the IDNum is bad
	// query if a sound request is playing now
	int IsPlayingNow(IDHANDLE IDNum);
	// query if play is over on a request
	int IsPlayOver(IDHANDLE IDNum);
	// query how long until request finishes	
	CMSFrameCode HowLongUntilFinished(IDHANDLE IDNum);
	// get the total request actually scheduled to be playing at this moment
	int TotalRequestsPlayingNow(void);
	// tell whether playing MONO or STEREO currently
	int PlayingStereoOrMono(void){
		return mStereoOrMonoOutput;
	}
	
};	 






#endif

