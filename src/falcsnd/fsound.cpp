#include <windows.h>
#include "fsound.h"
#include <mmreg.h>
#include "sim/include/stdhdr.h"
#include "falclib.h"
#include "token.h"
#include "dsound.h"
#include "psound.h"
#include "f4thread.h"
#include "soundfx.h"
#include "PlayerOp.h"
#include "sim/include/simdrive.h"
#include "sim/include/simlib.h"
#include "sim/include/otwdrive.h"
#include "sim/include/airframe.h"
#include "sim/include/aircrft.h"
#include "sim/include/dofsnswitches.h"
#include "voicemapper.h"
#include "sim/include/fcc.h"
#include "drawbsp.h"
#include "mlrVoice.h"
#include "profiler.h"

//MI for disabling VMS
#include "sim/include/aircrft.h"
extern bool g_bRealisticAvionics;
extern const char *FALCONSNDTABLETXT;
extern int g_nSoundUpdateMS;
extern bool g_bSoundHearVMSExternal;
extern bool g_bEnableDopplerSound, g_bSoundDistanceEffect, g_bNewEngineSounds;
extern bool g_bSoundSonicBoom;
bool g_bNoSound = false;

// F4SoundPos stuff
AList sndPurgeList;
F4CSECTIONHANDLE*    SoundPosSection; // Thread critical section information

#include "conv.h"
#include "VoiceManager.h"

LookupTable SonicBoomTable;

// ALL RESMGR CODE ADDITIONS START HERE
#define _USE_RES_MGR_ 1

#ifndef _USE_RES_MGR_ // DON'T USE RESMGR

#define FS_HANDLE FILE *
#define FS_OPEN   fopen
#define FS_READ   fread
#define FS_CLOSE  fclose
#define FS_SEEK   fseek
#define FS_TELL   ftell

#else // USE RESMGR

// #include "cmpclass.h"
extern "C"
{
#include "codelib/resources/reslib/src/resmgr.h"
}

#define FS_HANDLE FILE *
#define FS_OPEN   RES_FOPEN
#define FS_READ   RES_FREAD
#define FS_CLOSE  RES_FCLOSE
#define FS_SEEK   RES_FSEEK
#define FS_TELL   RES_FTELL

#endif

/*#include "simbase.h"
#include "campbase.h"
#include "awcsmsg.h"*/

// these are only (at least for the moment) for positional sounds
// sound group max volume levels can affect a group of sounds.
// right now the values are always at the max (or hacked in for test)
// presumably, this will hook into the UI's settings screen where there
// are vol sliders for different groups

// not needed anymore using PlayerOptions class
/*
float gGroupMaxVols[NUM_SOUND_GROUPS] =
{
0.0f,
0.0f,
0.0f,
0.0f,
};*/


WAVEFORMATEX mono_8bit_8k;
WAVEFORMATEX mono_8bit_22k;
WAVEFORMATEX mono_16bit_22k;
WAVEFORMATEX stereo_8bit_22k;
WAVEFORMATEX stereo_16bit_22k =
{
    WAVE_FORMAT_PCM,
    2,
    22050,
    88200,
    4,
    16,
    0x0000,
};
WAVEFORMATEX mono_16bit_8k =
{
    WAVE_FORMAT_PCM,
    1,
    8000,
    16000,
    2,
    16,
    0x0000,
};
LIST *sndHandleList;
VoiceFilter *voiceFilter = NULL;
//AWACSMessage *messageCenter;

// stuff for chat buffers
static const WORD SAMPLE_SIZE = 1; // Bytes per sample
static const DWORD SAMPLE_RATE = 8000; // Sample per second
typedef enum State { Receive = 0, Transmit, NotReady };
static State chatMode;
LPDIRECTSOUNDCAPTURE chatInputDevice;
extern "C" LPDIRECTSOUND DIRECT_SOUND_OBJECT;
// Chat IO stuff prototypes
BOOL ChatSetup(void);
void ChatCleanup(void);

long gSoundFlags = FSND_SOUND bitor FSND_REPETE;
static int soundCount = 0;

BOOL gSoundManagerRunning = FALSE;
extern char FalconObjectDataDir[_MAX_PATH];
extern char FalconSoundThrDirectory[_MAX_PATH];

#define MAX_FALCON_SOUNDS  16//8

// positional sound camera location
Trotation CamRot = IMatrix;
Tpoint CamPos;
Tpoint CamVel;

static void LoadSFX(char *falconDataDir);
static void UnLoadSFX(void);
static BOOL ReadSFXTable(char *sndtable);
static BOOL ReadSFXTableTXT(char *sndtable); // MLR 2003-10-17
static BOOL WriteSFXTable(char *sndtable);
BOOL SaveSFXTable();

// the global sound emmiter
F4SoundPos *gSoundObject = 0;


WAVEFORMATEX Mono_22K_8Bit =
{
    WAVE_FORMAT_PCM,
    1,
    22050,
    22050,
    1,
    8,
    0,
};

#if 0
WAVE Mono_8K_8Bit =
{
    0, 0, 0, 0,
    0l,
    0, 0, 0, 0, 0, 0, 0, 0,
    0l,
    WAVE_FORMAT_PCM,
    1,
    8000,
    8000,
    1,
    8,
    0, 0, 0, 0,
    0,
};

WAVE Mono_22K_16Bit =
{
    0, 0, 0, 0,
    0l,
    0, 0, 0, 0, 0, 0, 0, 0,
    0l,
    WAVE_FORMAT_PCM,
    1,
    22050,
    44100,
    2,
    16,
    0, 0, 0, 0,
    0,
};

WAVE Stereo_22K_8Bit =
{
    0, 0, 0, 0,
    0l,
    0, 0, 0, 0, 0, 0, 0, 0,
    0l,
    WAVE_FORMAT_PCM,
    2,
    22050,
    44100,
    2,
    8,
    0, 0, 0, 0,
    0,
};

WAVE Stereo_22K_16Bit =
{
    0, 0, 0, 0,
    0l,
    0, 0, 0, 0, 0, 0, 0, 0,
    0l,
    WAVE_FORMAT_PCM,
    2,
    22050,
    88200,
    4,
    16,
    0, 0, 0, 0,
    0,
};

char *ReadFile(char Filename[])
{
    char *Data;
    long size;
    DWORD bytesread;
    HANDLE wf;

    wf = CreateFile(Filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                    NULL);

    if (wf == INVALID_HANDLE_VALUE)
        return(NULL);

    size = GetFileSize(wf, NULL);
    // Data=(char *)malloc(size);
    Data = new char[size];

    if (Data == NULL)
        return(NULL);

    ReadFile(wf, Data, size, &bytesread, NULL);
    CloseHandle(wf);
    return(Data);
}

void InitWaveFormatEXData(void)
{
    mono_8bit_8k.wFormatTag = 0x0001;
    mono_8bit_8k.nChannels = 0x0001;
    mono_8bit_8k.nSamplesPerSec = 8000;
    mono_8bit_8k.nAvgBytesPerSec = 16000;
    mono_8bit_8k.nBlockAlign = 0x0002;
    mono_8bit_8k.wBitsPerSample = 16;
    mono_8bit_8k.cbSize = 0x0000;

    mono_8bit_22k.wFormatTag = 0x0001;
    mono_8bit_22k.nChannels = 0x0001;
    mono_8bit_22k.nSamplesPerSec = 0x00005622;
    mono_8bit_22k.nAvgBytesPerSec = 0x00005622;
    mono_8bit_22k.nBlockAlign = 0x0001;
    mono_8bit_22k.wBitsPerSample = 0x0008;
    mono_8bit_22k.cbSize = 0x0000;

    mono_16bit_22k.wFormatTag = 0x0001;
    mono_16bit_22k.nChannels = 0x0001;
    mono_16bit_22k.nSamplesPerSec = 0x00005622;
    mono_16bit_22k.nAvgBytesPerSec = 0x00005622;
    mono_16bit_22k.nBlockAlign = 0x0001;
    mono_16bit_22k.wBitsPerSample = 0x0010;
    mono_16bit_22k.cbSize = 0x0000;

    stereo_8bit_22k.wFormatTag = 0x0001;
    stereo_8bit_22k.nChannels = 0x0002;
    stereo_8bit_22k.nSamplesPerSec = 0x00005622;
    stereo_8bit_22k.nAvgBytesPerSec = 0x00005622;
    stereo_8bit_22k.nBlockAlign = 0x0001;
    stereo_8bit_22k.wBitsPerSample = 0x0008;
    stereo_8bit_22k.cbSize = 0x0000;

    stereo_16bit_22k.wFormatTag = 0x0001;
    stereo_16bit_22k.nChannels = 1;
    stereo_16bit_22k.nSamplesPerSec = 22050;
    stereo_16bit_22k.nAvgBytesPerSec = 44100;
    stereo_16bit_22k.nBlockAlign = 0x0002;
    stereo_16bit_22k.wBitsPerSample = 16;
    stereo_16bit_22k.cbSize = 0x0000;

    /* mono_16bit_8k.wFormatTag = 0x0001;
     mono_16bit_8k.nChannels = 0x0001;
     mono_16bit_8k.nSamplesPerSec = 0x00001f40;
     mono_16bit_8k.nAvgBytesPerSec = 0x00003e80;
     mono_16bit_8k.nBlockAlign = 0x0001;
     mono_16bit_8k.wBitsPerSample = 0x0010;
     mono_16bit_8k.cbSize = 0x0000;*/
    mono_16bit_8k.wFormatTag = 1;
    mono_16bit_8k.nChannels = 1;
    mono_16bit_8k.nSamplesPerSec = 8000;
    mono_16bit_8k.nAvgBytesPerSec = 16000;
    mono_16bit_8k.nBlockAlign = 2;
    mono_16bit_8k.wBitsPerSample = 16;
    mono_16bit_8k.cbSize = 0;
}

#endif

/* InitSoundManager:
 * Intergation of SndMgr -
 * Add SoundBegin()
 * Add ConversationBSegin()
 * Also need to create SoundManagerEnd() function
 * Add SoundEnd() and ConversationEnd()
 */
//int InitSoundManager (HWND hWnd, int mode, char *falconDataDir)
int InitSoundManager(HWND hWnd, int, char *falconDataDir)
{
    ShiAssert(FALSE == IsBadStringPtr(falconDataDir, _MAX_PATH));
    ShiAssert(FALSE not_eq IsWindow(hWnd));
    ShiAssert(FALSE == IsBadStringPtr(FalconObjectDataDir, _MAX_PATH));
    ShiAssert(FALSE == IsBadStringPtr(FalconSoundThrDirectory, _MAX_PATH));


    // Sonic Boom N wave
    SonicBoomTable.table[0].input   = 0;
    SonicBoomTable.table[0].output  = 1000;

    SonicBoomTable.table[1].input   = .19f;
    SonicBoomTable.table[1].output  = -2000;

    SonicBoomTable.table[2].input   = .20f;
    SonicBoomTable.table[2].output  = 1000;

    SonicBoomTable.table[3].input   =  1;
    SonicBoomTable.table[3].output  = -5000;

    SonicBoomTable.pairs = 4;



    if (gSoundDriver == NULL)
    {
        gSoundDriver = new CSoundMgr;

        if ( not gSoundDriver)
            return FALSE;
        else if ( not gSoundDriver->InstallDSound(hWnd, DSSCL_NORMAL, &stereo_16bit_22k))
        {
            delete gSoundDriver;
            gSoundDriver = NULL;
            return(FALSE);
        }
    }

    if (gSoundFlags bitand FSND_REPETE)
    {
        voiceFilter = new VoiceFilter;
        voiceFilter->SetUpVoiceFilter();
        voiceFilter->StartVoiceManager();
        // messageCenter = new AWACSMessage;
    }

    char sfxtable[_MAX_PATH];
    sprintf(sfxtable, "%s\\%s", FalconSoundThrDirectory, FALCONSNDTABLETXT);
    ShiAssert(SFX_DEF == NULL);

    if ( not ReadSFXTableTXT(sfxtable)) // MLR 2003-10-17 Parse text file if it exists
    {
        return FALSE;
        // MLR 2003-11-19 the new sound table is mandatory
        /*
        sprintf (sfxtable, "%s\\%s", FalconObjectDataDir, FALCONSNDTABLE);
        if (ReadSFXTable (sfxtable) == FALSE)
        {
         return FALSE;
        }*/
    }

    LoadSFX(falconDataDir);

    //JAM 14Dec03 - Fixing -nopete CTD
    if (gSoundFlags bitand FSND_REPETE)
        g_voicemap.SetVoiceCount(voiceFilter->fragfile.MaxVoices());

    gSoundManagerRunning = TRUE;

    SoundPosSection = F4CreateCriticalSection("SoundPosSection");


#if _MSC_VER >= 1300
    int i = _set_SSE2_enable(1);
#endif

    if ( not gSoundObject) // MLR 1/25/2004 - the global sound object
    {
        gSoundObject = new F4SoundPos();
    }

    return(TRUE);
}

// Hook to save the sfx tbl
BOOL SaveSFXTable()
{
    char sfxtable[_MAX_PATH];

    if (FalconObjectDataDir == NULL) return FALSE;

    sprintf(sfxtable, "%s\\%s", FalconObjectDataDir, FALCONSNDTABLE);
    ShiAssert(SFX_DEF == NULL);

    if (WriteSFXTable(sfxtable) == FALSE)
        return FALSE;

    return TRUE;
}

/* Load Sound Loads a wave file and returns a handle to the sound
 * Update this with sound manager calls
 * SND_EXPORT int AudioLoad( char * filename )
 * need to research more about the existing variable Flags
 * and determine what functionality will be needed for it
 */
int F4LoadSound(char filename[], long Flags)
{
    int SoundID = SND_NO_HANDLE;

    if (gSoundDriver)
        SoundID = gSoundDriver->LoadWaveFile(filename, Flags, NULL);

    return(SoundID);
}

int F4LoadFXSound(char filename[], long Flags, SFX_DEF_ENTRY *sfx)
{
    int SoundID = SND_NO_HANDLE;

    if (gSoundDriver)
        SoundID = gSoundDriver->LoadWaveFile(filename, Flags, sfx);

    return(SoundID);
}

/*
 * Same as function above, except for raw sounds
 * These two functions, load the file and fill the buffer
 * Does not play sound immediately. The load raw function will
 * have to be updated to the file. Wavs have the load audio which
 * contains the load wav function. The raw will be based off the same
 * with the exception of a generic load file and have the Wav header
 * slapped on. 8bit-22k
 */
// MLR apparently is never used
//int F4LoadRawSound (int flags, char *data, int len)
int F4LoadRawSound(int, char *data, int len)
{
    int SoundID = SND_NO_HANDLE;

    if (gSoundDriver)
        SoundID = gSoundDriver->AddRawSample(&Mono_22K_8Bit, data, len, 0);

    return(SoundID);
}

/*
 * Funcionality: free a sound loaded
 */
void F4FreeSound(int* sound)
{
    if (gSoundDriver)
        gSoundDriver->RemoveSample(*sound);

    *sound = SND_NO_HANDLE;
}

/*
 * Like the function says.
 *  Need to know if these functions are what they are looking for
 or if the functionality should be more a stop - the sound if started
 again would start at the beginning. Pause being, resume from point
 of pause.
    SoundPause // This stop is more of a pause
    SoundResume

SND_EXPORT int SoundStopChannel( AUDIO_CHANNEL * channel )
 */
void F4SoundStop()
{
    if (gSoundDriver)
    {
        gSoundDriver->StopAllSamples();
    }
}

/* Assume that a sound loaded will call this to execute play
 * assume its more of a resume
 */
void F4SoundStart()
{
    if (gSoundDriver)
    {
        F4SoundFXInit();
    }
}

/* Must replace the CreateStream function call with
 AUDIO_ITEM *tlkItem = NULL;
 int dummyHandle;

 dummyHandle = AudioCreate( &( waveFormat ) );
 audioChannel = SoundRequestChannel( dummyHandle,
  &( audioHandle ), DSB_SIZE );

 tlkItem = AudioGetItem( audioHandle );

 if ( audioChannel )
 SoundStreamChannel( audioChannel, fillSoundBuffer );
 *
 */
/*int F4CreateStream(int Flags)
{
 int StreamID=SND_NO_HANDLE;

 if(gSoundDriver)
 {
 switch(Flags)
 {
 case 1: // 22k 8bit mono
 StreamID=gSoundDriver->CreateStream(&Mono_22K_8Bit,0.5f);
 break;
 case 2: // 22k 16bit mono
 StreamID=gSoundDriver->CreateStream(&Mono_22K_16Bit,0.5f);
 break;
 case 3: // 22k 8bit stereo
 StreamID=gSoundDriver->CreateStream(&Stereo_22K_8Bit,0.5f);
 break;
 case 4: // 22k 16bit stereo
 StreamID=gSoundDriver->CreateStream(&Stereo_22K_16Bit,0.5f);
 break;
 }
 }
 return(StreamID);
}
*/

int F4CreateStream(WAVEFORMATEX *fmt, float seconds)
{
    if (gSoundDriver)
        return(gSoundDriver->CreateStream(fmt, seconds));

    return(SND_NO_HANDLE);
}

/*
 * Need to know if this is similar to the SoundReleaseChannel Call
 */
void F4RemoveStream(int StreamID)
{
    if (gSoundDriver and StreamID not_eq SND_NO_HANDLE)
        gSoundDriver->RemoveStream(StreamID);
}

/* Is this merely a play channel call for a sound in a stream?
 * handle1 = AudioStream( "c:\\msdev\\compdata\\stream.wav", -1 );
 */
int F4StartStream(char *filename, long flags)
{
    WAVEFORMATEX Header;
    int StreamID = SND_NO_HANDLE;
    long size, NumSamples;

    if (gSoundDriver)
    {
        // Start KLUDGE
        gSoundDriver->LoadRiffFormat(filename, &Header, &size, &NumSamples);

        if (Header.wFormatTag == WAVE_FORMAT_IMA_ADPCM)
        {
            Header.wFormatTag = WAVE_FORMAT_PCM;
            Header.wBitsPerSample *= 4;
            Header.nBlockAlign = (unsigned short)(Header.nChannels * Header.wBitsPerSample / 8);
            Header.nAvgBytesPerSec = Header.nSamplesPerSec * Header.nBlockAlign;
        }

        StreamID = gSoundDriver->CreateStream(&Header, 0.5);

        // END KLUDGE
        if (StreamID not_eq SND_NO_HANDLE)
        {
            if (gSoundDriver->StartFileStream(StreamID, filename, flags))
                return(StreamID);
            else
                gSoundDriver->RemoveStream(StreamID);
        }
    }

    return(SND_NO_HANDLE);
}

/* Is this merely a repeat channel call for a sound in a stream?
*/
BOOL F4LoopStream(int StreamID, char *filename)
{
    if (gSoundDriver and StreamID not_eq SND_NO_HANDLE)
        return(gSoundDriver->StartFileStream(StreamID, filename, SND_STREAM_LOOP));

    return(FALSE);
}

BOOL F4StartRawStream(int StreamID, char *Data, long size)
{
    if (gSoundDriver and StreamID not_eq SND_NO_HANDLE)
        return(gSoundDriver->StartMemoryStream(StreamID, Data, size));

    return(FALSE);
}

BOOL F4StartCallbackStream(int StreamID, void *ptr, DWORD (*cb)(void *, char *, DWORD))
{
    if (gSoundDriver and StreamID not_eq SND_NO_HANDLE)
        return(gSoundDriver->StartCallbackStream(StreamID, ptr, cb));

    return(FALSE);
}

void F4StopStream(int StreamID)
{
    if (gSoundDriver and StreamID not_eq SND_NO_HANDLE)
    {
        gSoundDriver->StopStream(StreamID);
        gSoundDriver->RemoveStream(StreamID);
    }
}

void F4StopAllStreams()
{
    // add here reset for lists
    /* This I must kill the voices here in buffer*/
    if (voiceFilter)
    {
        voiceFilter->ResetVoiceManager();
    }

    /* Also the following just stops and I need to play them here
     */
    if (gSoundDriver)
        gSoundDriver->StopAllStreams();
}
/*
void F4PlayVoiceStreams()
{
 if(voiceFilter)
 voiceFilter->ResumeVoiceStreams();
}*/

long F4SetStreamVolume(int ID, long vol)
{
    if (gSoundDriver)
        return(gSoundDriver->SetStreamVolume(ID, vol));

    return(-10000);
}

void F4HearVoices()
{
    if (voiceFilter)
        voiceFilter->HearVoices();
}

void F4SilenceVoices()
{
    if (voiceFilter)
        voiceFilter->SilenceVoices();
}

long F4StreamPlayed(int StreamID)
{
    if (gSoundDriver)
        return(gSoundDriver->GetStreamPlayTime(StreamID));

    return(0);
}
// Direction (dBs) =-10000 to 10000 where -=Left,0=Center,+=Right (This is a Percentage)
/* Update this with the audio function
 SND_EXPORT int AudioSetPan( int handle, int location );
 * There is also a linear pan function AudioSetPanRate
 */
void F4PanSound(int soundIdx, int PanDir)
{
    if (gSoundDriver and soundIdx not_eq SND_NO_HANDLE)
        gSoundDriver->SetSamplePan(soundIdx, PanDir);
}

// Pitch = 0.1 to 10.0 where 1.0 is the original Pitch (ie Frequency)
/* Use audio function
SND_EXPORT int AudioSetPitch( int handle, int pitch )
 *
 */
void F4PitchBend(int soundIdx, float Pitch)
{
    if (gSoundDriver and soundIdx not_eq SND_NO_HANDLE)
        gSoundDriver->SetSamplePitch(soundIdx, Pitch);
}

/* Use audio function
 * AudioSetVolume
 */
// Volume is in dBs (-10000 -> 0)
long F4SetVolume(int soundIdx, int Volume)
{
    if (gSoundDriver and soundIdx not_eq SND_NO_HANDLE)
        return(gSoundDriver->SetSampleVolume(soundIdx, Volume));

    return(-10000);
}

// Volume is in dBs (-10000 -> 0)
void F4SetGroupVolume(int group, int vol)
{
    if (group < 0 or group >= NUM_SOUND_GROUPS)
        return;

    //gGroupMaxVols[ group ] = vol;
    PlayerOptions.GroupVol[ group ] = vol;
}

void F4SetSoundFlags(int soundIdx, long flags)
{
    SOUNDLIST *snd;

    if (gSoundDriver and soundIdx not_eq SND_NO_HANDLE)
    {
        snd = gSoundDriver->FindSample(soundIdx);

        if (snd not_eq NULL)
            snd->Flags or_eq flags;
    }
}

//void F4SetStreamFlags(int soundIdx,long flags)
void F4SetStreamFlags(int, long)
{
    /*
     if(gSoundDriver and soundIdx not_eq SND_NO_HANDLE)
     {
     }
    */
}

int F4GetVolume(int soundIdx)
{
    if (gSoundDriver and soundIdx not_eq SND_NO_HANDLE)
        return(gSoundDriver->GetSampleVolume(soundIdx));

    return(0);
}
/* Use audio function
 * SND_EXPORT void AudioPlay( int handle )
 * possible use the SoundPlayChannel: need to know the level of interaction they
 * want.
 */
void F4PlaySound(int soundIdx)
{
    if (gSoundDriver and soundIdx not_eq SND_NO_HANDLE)
        gSoundDriver->PlaySample(soundIdx, 0);
}

void F4PlaySound(int soundIdx, int flags)
{
    if (gSoundDriver and soundIdx not_eq SND_NO_HANDLE)
        gSoundDriver->PlaySample(soundIdx, flags);
}

/* AudioCheck if playing
 * SND_EXPORT int AudioIsPlaying( int handle )
 */
int F4IsSoundPlaying(int theSound, int UID)
{
    if (gSoundDriver and theSound not_eq SND_NO_HANDLE)
        return(gSoundDriver->IsSamplePlaying(theSound, UID));

    return(0);
}

int F4SoundFXPlaying(int sfxId, int UID)
{
    ShiAssert(sfxId < NumSFX);
    ShiAssert(sfxId > 0);

    if (sfxId <= 0 or sfxId >= NumSFX) return 0;

    if (gSoundManagerRunning == FALSE) return 0;

    return F4IsSoundPlaying(SFX_DEF[sfxId].handle, UID);
}

/*
 * SND_EXPORT int AudioSetLoop( int handle, int loop_count )
 */
void F4LoopSound(int soundIdx)
{
    if (gSoundDriver and soundIdx not_eq SND_NO_HANDLE)
        //gSoundDriver->PlaySample(soundIdx,SND_LOOP_SAMPLE bitor SND_EXCLUSIVE);
        gSoundDriver->PlaySample(soundIdx, SFX_POS_LOOPED);
}

/* Need to know the differenced between different stops
 * Soundstop, stopstream, stopsound, Q: will the stop channel
 * be sufficient for all of this? This stop sound is stop, not pause
 */
void F4StopSound(int soundIdx)
{
    if (gSoundDriver and soundIdx not_eq SND_NO_HANDLE)
        gSoundDriver->StopSample(soundIdx);
}

/* Exit Sound Manager: Add the SoundEnd() function and the
 * the Conversation function here
 */
void ExitSoundManager(void)
{

    if (gSoundObject)
    {
        delete gSoundObject;
        gSoundObject = 0;
    }

    F4DestroyCriticalSection(SoundPosSection);

    gSoundManagerRunning = FALSE;

    if (gSoundDriver)
    {
        // delete messageCenter;
        delete voiceFilter;
        voiceFilter = NULL;
        gSoundDriver->RemoveAllSamples();
        gSoundDriver->RemoveAllStreams();
        gSoundDriver->RemoveDSound();
        delete gSoundDriver;
        gSoundDriver = NULL;
    }

    UnLoadSFX();

    if (SFX_DEF not_eq BuiltinSFX)
    {
        delete [] SFX_DEF;
    }

    SFX_DEF = NULL;
}

//void PlayRadioMessage( int talker, int msgid, int *data );
/* Need to find out what the purpose of this callback is.
*/
//void CALLBACK SoundHandler (UINT a, UINT b, DWORD c, DWORD d, DWORD e)
void CALLBACK SoundHandler(UINT, UINT, DWORD, DWORD, DWORD)
{
}

// JPO - read in the sound fx table
BOOL ReadSFXTable(char *sndtable)
{
    ShiAssert(FALSE == IsBadStringPtr(sndtable, 256));
    FILE *fp = fopen(sndtable, "rb");

    if (fp == NULL)
    {
        SFX_DEF = BuiltinSFX;
        NumSFX = BuiltinNSFX;
        return TRUE;
    }

    UINT nsfx;
    int vrsn;

    if (fread(&vrsn, sizeof(vrsn), 1, fp) not_eq 1 or
        fread(&nsfx, sizeof(nsfx), 1, fp) not_eq 1)
    {
        SFX_DEF = BuiltinSFX;
        NumSFX = BuiltinNSFX;
        fclose(fp);
        return TRUE;
    }

    ShiAssert(nsfx >= SFX_LAST and nsfx < 32767); // arbitrary test

    if (nsfx < SFX_LAST)
    {
        ShiWarning("Out of date SFX table");
        SFX_DEF = BuiltinSFX;
        NumSFX = BuiltinNSFX;
        fclose(fp);
        return TRUE;
    }

    if (vrsn not_eq SFX_TABLE_VRSN)
    {
        ShiWarning("Old Version of Sound Table");
        SFX_DEF = BuiltinSFX;
        NumSFX = BuiltinNSFX;
        fclose(fp);
        return TRUE;
    }

    SFX_DEF = new SFX_DEF_ENTRY[nsfx];

    if (fread(SFX_DEF, sizeof(*SFX_DEF), nsfx, fp) not_eq nsfx)
    {
        ShiAssert( not "Read error on Sound Table");
        fclose(fp);
        return FALSE;
    }

#if 0 // mlr used to export sound data
    {
        int i;
        FILE *t;

        t = fopen("f4sndtbl.txt", "w");

        char flags[] = {"PLEV3FAHOR"};

        for (i = 0; i < nsfx; i++)
        {
            fprintf(t, "# %d\n", i);
            fprintf(t, "%s\t%d\t%d\t%f\t%f\t%f\t%f\t",
                    SFX_DEF[i].fileName,
                    SFX_DEF[i].offset,
                    SFX_DEF[i].length,
                    SFX_DEF[i].maxDistSq,
                    SFX_DEF[i].min3ddist,
                    SFX_DEF[i].maxVol,
                    SFX_DEF[i].minVol
                   );

            int l;

            for (l = 0; l < sizeof(flags); l++)
            {
                if (SFX_DEF[i].flags bitand 1 << l)
                    fprintf(t, "%s", flags[l]);
            }

            fprintf(t, "\t%f\t%d\t%d\t%d\n",
                    SFX_DEF[i].pitchScale,
                    SFX_DEF[i].soundGroup,
                    SFX_DEF[i].InternalID,
                    SFX_DEF[i].Unused
                    // SFX_DEF[i].majorSymbol,
                    // SFX_DEF[i].minorSymbol
                   );
        }

        fclose(t);
    }
#endif


    NumSFX = nsfx;
    fclose(fp);
    return TRUE;
}


// MLR 2003-10-18 Read sound table from text file


extern bool g_bEnableDopplerSound, g_bSoundDistanceEffect;
extern float g_fSoundDopplerFactor, g_fSoundRolloffFactor;
extern int g_nSoundUpdateMS;



BOOL ReadSFXTableTXT(char *sndtable)
{
    char buffer[512], *arg;
    ShiAssert(FALSE == IsBadStringPtr(sndtable, 256));
    FILE *fp = fopen(sndtable, "r");

    if (fp == NULL)
    {
        return FALSE;
    }

    int nsfx = 0;

    while (fgets(buffer, 512, fp))
    {
        if (buffer[0] not_eq '#')
            nsfx++;
    }

    fseek(fp, 0, SEEK_SET);

    SFX_DEF = new SFX_DEF_ENTRY[nsfx];
    NumSFX = nsfx;

    int i = 0;

    while (fgets(buffer, 512, fp))
    {
        if (buffer[0] not_eq '#')
        {
            arg = strtok(buffer, " ,\t\n");
            strncpy(SFX_DEF[i].fileName, arg, 64);
            SFX_DEF[i].offset = TokenI(0, 0);
            SFX_DEF[i].length = TokenI(0, 0);
            SFX_DEF[i].handle = 0; // run time data
            SFX_DEF[i].maxDistSq = TokenF(0, 50000);
            SFX_DEF[i].maxDistSq *= SFX_DEF[i].maxDistSq; // Must square values
            SFX_DEF[i].min3ddist = TokenF(0, 0);
            SFX_DEF[i].min3ddist *= SFX_DEF[i].min3ddist; // Must square values
            SFX_DEF[i].maxVol = TokenF(0, 0);
            SFX_DEF[i].minVol = TokenF(0, -1000);
            SFX_DEF[i].distSq = 0; // TokenI(0,0); run time data
            SFX_DEF[i].override = 0; // TokenI(0,0); run time data
            SFX_DEF[i].lastFrameUpdated = 0; // TokenI(0,0); run time data
            //SFX_DEF[i].flags=TokenI(0,0);
            arg = strtok(0, " ,\t\n");
            SFX_DEF[i].flags = 0;

            if (arg)
            {
                while (*arg)
                {
                    int l;
                    char flags[16] = "PLEV3FAHORSICOX";

                    for (l = 0; l < 16; l++)
                    {
                        if (*arg == flags[l])
                        {
                            SFX_DEF[i].flags or_eq 1 << l;
                        }
                    }

                    arg++;
                }
            }


            if (SFX_DEF[i].flags bitand (SFX_POS_SELF bitor SFX_POS_EXTONLY bitor SFX_POS_EXTINT))
            {
                // for all those types, set the External flag
                SFX_DEF[i].flags or_eq SFX_POS_EXTERN;
            }

            if (SFX_DEF[i].flags bitand SFX_POS_EXTERN)
            {
                // for all external types, set the 3d flag
                SFX_DEF[i].flags or_eq SFX_FLAGS_3D;
                // SFX_DEF[i].flags or_eq SFX_FLAGS_FREQ; // needed for doppler effect // this will be handled in psound

            }

            SFX_DEF[i].pitchScale = TokenF(0);
            SFX_DEF[i].soundGroup = TokenI(0);
            SFX_DEF[i].LinkedSoundID = TokenI(0);
            SFX_DEF[i].Unused = TokenI(0);
            SFX_DEF[i].coneInsideAngle  = cos(TokenF(0)   / 180 * PI);
            SFX_DEF[i].coneOutsideAngle = cos(TokenF(180) / 180 * PI);
            SFX_DEF[i].coneOutsideVol   = TokenF(SFX_DEF[i].maxVol);

            // SFX_DEF[i].majorSymbol=TokenI(0,0);
            // SFX_DEF[i].minorSymbol=TokenI(0,0);
            i++;
        }
    }

    // if the text file is too short, get defaults
    while (i < BuiltinNSFX)
    {
        SFX_DEF[i] = BuiltinSFX[i];
        i++;
    }

    fclose(fp);

    //#define SNDINI
#ifdef SNDINI
    {
        FILE *fp;

        if (fp = fopen("f4sound.ini", "w"))
        {
            fprintf(fp,
                    "id=%d"
                    "filename=%s"
                    "{\nSound ID-%d \"%s\" Offset=%d Lenght=%d MaxDist=%f MinDist=%f MaxVol=%f MinVol=%f Flags=%8x ",
                    i,
                    SFX_DEF[i].fileName,
                    SFX_DEF[i].offset,
                    SFX_DEF[i].length,
                    SFX_DEF[i].maxDistSq,
                    SFX_DEF[i].min3ddist,
                    SFX_DEF[i].maxVol,
                    SFX_DEF[i].minVol,
                    SFX_DEF[i].flags);

            int l;

            for (l = 0; l < 10; l++)
            {
                char flags[11] = "PLEV3FAHOR";

                if (SFX_DEF[i].flags bitand 1 << l)
                    fprintf(fp, "%c", flags[l]);
            }

            fprintf(fp, "\n");

            fclose(fp);



        }
    }
#endif

    return TRUE;
}

extern char FalconDataDirectory[_MAX_PATH];

// MLR Hack to allow reloading the sound data while in the 3d world
// to make dev'ing the sound data easier.
// Odds are, if it failes, your screwed.
void F4ReloadSFX(void)
{
    UnLoadSFX();

    if (SFX_DEF not_eq BuiltinSFX)
    {
        delete [] SFX_DEF;
    }

    SFX_DEF = NULL;

    char sfxtable[_MAX_PATH];
    sprintf(sfxtable, "%s\\%s", FalconSoundThrDirectory, FALCONSNDTABLETXT);

    if ( not ReadSFXTableTXT(sfxtable)) // MLR 2003-10-17 Parse text file if it exists
    {
        return; // MLR 2003-11-18 - the new style sound table is mandatory
        /*
        sprintf (sfxtable, "%s\\%s", FalconObjectDataDir, FALCONSNDTABLE);
        if (ReadSFXTable (sfxtable) == FALSE)
        {
         // screwed
         return;
        }
        */
    }

    LoadSFX(FalconDataDirectory);
}


void LoadSFX(char *falconDataDir)
{
    int i;
    char fname[MAX_PATH];

    // RV - Biker - Suppress log-file
    //sprintf( fname, "%s\\%s", FalconSoundThrDirectory, "SoundError.log" );

    //FILE *fp;
    //fp=fopen("SoundError.log","w");
    {

        for (i = 0; i < NumSFX; i++) // first pass - most important buffers
        {
            if ((SFX_DEF[i].flags bitand SFX_FLAGS_HIGH) == 0)
                continue;

            sprintf(fname, "%s\\%s", FalconSoundThrDirectory, SFX_DEF[i].fileName);
            //SFX_DEF[i].handle = F4LoadFXSound(fname, SND_EXCLUSIVE, &SFX_DEF[i]);
            SFX_DEF[i].handle = F4LoadFXSound(fname, SFX_DEF[i].flags, &SFX_DEF[i]);
            //if(SFX_DEF[i].handle == SND_NO_HANDLE)
            //{
            // if(fp)
            // fprintf(fp,"LoadSFX() didn't load %d:%s\n",i,fname);
            //}
            // ShiAssert (SFX_DEF[i].handle not_eq SND_NO_HANDLE); // MLR 1/21/2004 - who cares
        }

        for (i = 0; i < NumSFX; i++)
        {
            if (SFX_DEF[i].flags bitand SFX_FLAGS_HIGH)
                continue;

            sprintf(fname, "%s\\%s", FalconSoundThrDirectory, SFX_DEF[i].fileName);
            //SFX_DEF[i].handle = F4LoadFXSound(fname, SND_EXCLUSIVE, &SFX_DEF[i]);
            SFX_DEF[i].handle = F4LoadFXSound(fname, SFX_DEF[i].flags, &SFX_DEF[i]);
            //if(SFX_DEF[i].handle == SND_NO_HANDLE)
            //{
            // if(fp)
            // fprintf(fp,"LoadSFX() didn't load %f\n",fname);
            //}
            // ShiAssert (SFX_DEF[i].handle not_eq SND_NO_HANDLE); // MLR 1/21/2004 - who cares
        }

        //if(fp)
        // fclose(fp);
    }
}

// JPO - write out the sound fx table
BOOL WriteSFXTable(char *sndtable)
{
    ShiAssert(FALSE == IsBadStringPtr(sndtable, _MAX_PATH));

    if (BuiltinNSFX <= 0 or BuiltinSFX == NULL) return FALSE;

    ShiAssert(FALSE == F4IsBadReadPtr(BuiltinSFX, sizeof(SFX_DEF) * BuiltinNSFX));

    FILE *fp = fopen(sndtable, "wb");

    if (fp == NULL)
    {
        return FALSE;
    }

    int vrsn = SFX_TABLE_VRSN;

    if (fwrite(&vrsn, sizeof(vrsn), 1, fp) not_eq 1 or
        fwrite(&BuiltinNSFX, sizeof(BuiltinNSFX), 1, fp) not_eq 1)
    {
        ShiAssert( not "Write error on Sound Table");
        fclose(fp);
        return FALSE;
    }

    if (fwrite(BuiltinSFX, sizeof(*BuiltinSFX), BuiltinNSFX, fp) not_eq (UINT)BuiltinNSFX)
    {
        ShiAssert( not "Write error on Sound Table");
        fclose(fp);
        return FALSE;
    }

    fclose(fp);
    return TRUE;
}

void UnLoadSFX(void)
{
    int i;

    for (i = 0; i < NumSFX; i++)
    {
        F4FreeSound(&(SFX_DEF[i].handle));
        ShiAssert(SFX_DEF[i].handle == SND_NO_HANDLE);
    }
}

/*
** Name: ChatSetup
** Description:
** Setup the chat buffer and kick off other chat stuff...
*/
BOOL
ChatSetup(void)
{
    if ( not gSoundDriver) return(FALSE);

#ifdef CHAT_USED
    HRESULT result;
    WAVEFORMATEX audioFormat;

    // Setup the audio format structure we want
    /*
    audioFormat.wFormatTag = WAVE_FORMAT_PCM;
    audioFormat.nChannels = 1;
    audioFormat.nSamplesPerSec = SAMPLE_RATE;
    audioFormat.nAvgBytesPerSec = SAMPLE_RATE * SAMPLE_SIZE;
    audioFormat.nBlockAlign = SAMPLE_SIZE;
    audioFormat.wBitsPerSample = 8 * SAMPLE_SIZE;
    audioFormat.cbSize = 0;
    */

    audioFormat.wFormatTag = SND_FORMAT;
    audioFormat.nChannels = SND_CHANNELS;
    audioFormat.nSamplesPerSec = SND_SAMPLE_RATE;
    audioFormat.nAvgBytesPerSec = SND_AVG_RATE;
    audioFormat.nBlockAlign = SND_BLOCK_ALIGN;
    audioFormat.wBitsPerSample = SND_BIT_RATE;
    audioFormat.cbSize = 0;


    // Setup input stuff
    result = DirectSoundCaptureCreate(NULL, &chatInputDevice, NULL);
    DSErrorCheck(result);


    // Setup our service modules
    SetupTalkIO(chatInputDevice, DIRECT_SOUND_OBJECT, &audioFormat);

    // Fake that we're transmitting
    chatMode = Transmit;

    // Now toggle into receive mode
    F4ChatToggleXmitReceive();

#endif

    return TRUE;
}


/*
** Name: ChatCleanup
** Description:
** Cleanup chat stuff...
*/
void
ChatCleanup(void)
{
#ifdef CHAT_USED
    HRESULT result;

    if ( not gSoundDriver) return;


    // If we were transmitting, stop
    if (chatMode == Transmit)
    {
        F4ChatToggleXmitReceive();
    }

    // Cleanup our service modules
    CleanupTalkIO();


    // Clean up input stuff
    result = chatInputDevice->Release();
    DSErrorCheck(result);
#endif

}


void
F4ChatToggleXmitReceive(void)
{
    if ( not gSoundDriver) return;

    // Switch modes
    if (chatMode == Transmit)
    {
        chatMode = Receive;
        // KCK: Commented out - need to use new VoiceDataMessage
        // EndTransmission();
    }
    else
    {
        chatMode = Transmit;
        // KCK: Commented out - need to use new VoiceDataMessage
        // BeginTransmission();
    }
}

// called by OTWDrive when entering the 3d world
// going to init some stuff

void F4SoundEntering3d(void)
{
    g_bNewEngineSounds = false;
    g_bEnableDopplerSound  = false;
    g_bSoundDistanceEffect = false;
    g_bSoundHearVMSExternal = false;

    if (PlayerOptions.SoundFlags bitand SNDFNEWENG)
    {
        g_bNewEngineSounds  = true;
    }

    if (PlayerOptions.SoundFlags bitand SNDFDOP)
    {
        g_bEnableDopplerSound  = true;
    }

    if (PlayerOptions.SoundFlags bitand SNDFDISTE)
    {
        g_bSoundDistanceEffect = true;
    }

    if (PlayerOptions.SoundFlags bitand SNDFVMSEXT)
    {
        g_bSoundHearVMSExternal = true;
    }
}

void F4SoundLeaving3d(void)
{
    F4SoundStop();
    gVoiceManager.StopAll();
}


/*
** Name: F4SoundSetCamPosAndOrient
** Description:
** Should be called at the start of the otwframe to set the
** Camera position and orientation.
*/

float ExtAttenuation = -10000;

extern "C" void F4SoundFXSetCamPosAndOrient(Tpoint *campos, Trotation *camrot, Tpoint *camvel)
{
    CamPos = *campos;
    CamRot = *camrot;
    CamVel = *camvel;

    ExtAttenuation = 0;

    // MonoPrint("InCockpit:%d\n",OTWDriver.DisplayInCockpit());
    if (OTWDriver.DisplayInCockpit())
    {

        if (SimDriver.GetPlayerEntity())
        {
            float v;
            AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
            v = playerAC->af->GetSoundExternalVol() + PlayerOptions.SoundExtAttenuation;

            if (
                playerAC->GetNumDOFs() > COMP_CANOPY_DOF and 
                playerAC->IsComplex() and 
                playerAC->af->GetCanopyMaxAngle()
            )
            {
                ExtAttenuation = (1 - (playerAC->GetDOFValue(COMP_CANOPY_DOF) /
                                       (playerAC->af->GetCanopyMaxAngle() * DTR))) * v;
            }
            else
            {
                if (playerAC->af->canopyState == false)
                {
                    // closed
                    ExtAttenuation = v;
                }
            }
        }
        else
        {
            // presume closed
            //ExtAttenuation=-2000;
            // Cobra - Should use player-selected attentuation
            ExtAttenuation = (float)PlayerOptions.SoundExtAttenuation;
        }
    }
}

/*
** Name: F4SoundSetPos
** Description:
** Sets a sound's position in the world.  We use the camera location
** to get distance squared to camera and then set the distance in
** the effects table.
*/

extern "C" void
F4SoundFXSetPos(int sfxId, int override,
                float x,  float y,  float z,
                float pscale, float volume ,
                int   uid)
{
    if (gSoundObject)
        gSoundObject->Sfx(sfxId, uid, pscale, volume, x, y, z);
}

extern "C" void
F4SoundFXSetDist(int sfxId, int override, float volume, float pscale)
{
    //Cobra Inhibit stuff?
    SFX_DEF_ENTRY *sfxp;

    if (sfxId <= 0 or sfxId >= NumSFX) return;

    sfxp = &SFX_DEF[ sfxId ];

    // Cobra - Fix CTD when exiting FF
    if (F4IsBadReadPtr(sfxp, sizeof(sfxp))) return;

    if (g_bRealisticAvionics and 
        (sfxp->flags bitand SFX_FLAGS_VMS))
    {
        AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

        //MI no VMS when on ground
        if (playerAC)
        {
            if (
                OTWDriver.DisplayInCockpit() and playerAC->OnGround() or
 not playerAC->playBetty or not playerAC->IsSetFlag(MOTION_OWNSHIP)
            )
            {
                // MD -- 20031125: except if the MAL/IND test button is being pressed to test the warning sound
                if ( not ((sfxId == SFX_BB_ALLWORDS) and playerAC->TestLights))
                {
                    return;
                }
            }
        }
    }

    //End

    if (gSoundObject)
    {
        if ((SFX_DEF[ sfxId ].flags bitand SFX_POS_LOOPED) or
            override or
            ( not gSoundObject->IsPlaying(sfxId, 0)))
            gSoundObject->Sfx(sfxId, 0, pscale, volume, CamPos.x, CamPos.y, CamPos.z);
    }
}

// this variable and mask is used to stagger positional volume setting
// for looping sounds.  mask = 7 is every 8th frame
// mask = 3 is every 4th frame
static unsigned int sPosLoopStagger = 0;
#define LOOP_STAGGER_MASK 0x00000003U

/*
** Name: F4SoundPositionDriver
** Description:
** Goes thru the list of sound effects and determines if they should be
** started/stopped or volume increased/decreased.
*/
extern "C" void
F4SoundFXPositionDriver(unsigned int begFrame, unsigned int endFrame)
{
    static unsigned long lastPlayTime = 0;
    //int i;
    //SFX_DEF_ENTRY *sfxp;
    //BOOL isPlaying;
    //int volLevel;
    //float volRange;
    //float maxVol;
    //int curVolLevel;

    // Maximum of 20 Hz update on the sound
    if ((vuxRealTime - lastPlayTime) > (VU_TIME)(g_nSoundUpdateMS))
    {
        lastPlayTime = vuxRealTime;

        if (gSoundManagerRunning == FALSE or not SimDriver.InSim())
        {
            return;
        }

        // set stagger counter
        sPosLoopStagger = (++sPosLoopStagger) bitand LOOP_STAGGER_MASK;

        if (gSoundDriver)
        {
            static int LastOTWDispMode = -1;
            bool reset = 0;

            if (OTWDriver.GetOTWDisplayMode() not_eq LastOTWDispMode)
            {
                reset = 1;
                LastOTWDispMode = OTWDriver.GetOTWDisplayMode();
            }

            {
                F4SoundPos *sp;

                // MLR 5/15/2004 - this will prevent the sonic boom from occuring
                // due to view changes
                F4SoundPos::OTWViewChanged(reset);

                sp = (F4SoundPos *)sndPurgeList.GetHead();

                while (sp)
                {
                    sp->PositionalData();
                    sp = (F4SoundPos *)sp->GetSucc();
                }
            }

            gVoiceManager.Exec(&CamPos, &CamRot, &CamVel);

            //gSoundDriver->AssignSamples();
            gSoundDriver->SetCameraPostion(&CamPos, &CamRot, &CamVel, reset);

            F4SoundPos *sp, *sp2;
            sp = (F4SoundPos *)sndPurgeList.GetHead();

            while (sp)
            {
                sp2 = (F4SoundPos *)sp->GetSucc();
                sp->Purge();
                sp = sp2;
            }
        }
    }
}


/*
** Name: F4SoundFXInit
** Description:
** Inits soundfx variables
*/
extern "C" void
F4SoundFXInit(void)
{
    int i;
    SFX_DEF_ENTRY *sfxp;

    if ( not gSoundDriver) return;

    // main loop thru sound effects
    for (i = 0, sfxp = &SFX_DEF[0]; i < NumSFX; i++, sfxp++)
    {
        sfxp->distSq = sfxp->maxDistSq;
        sfxp->lastFrameUpdated = 0;
    }
}


/*
** Name: F4SoundFXEnd
** Description:
** Stops all sounds from playing
*/
extern "C" void
F4SoundFXEnd(void)
{
    int i;
    SFX_DEF_ENTRY *sfxp;

    if ( not gSoundDriver) return;

    // main loop thru sound effects
    for (i = 0, sfxp = &SFX_DEF[0]; i < NumSFX; i++, sfxp++)
    {
        if (F4IsSoundPlaying(sfxp->handle))
            F4StopSound(sfxp->handle);
    }
}

#include "../vu2/include/vuentity.h"


// used internally to get unique IDs, each object actually gets 100 UIDs
int F4SoundPosUID = 100;

#define ENTERSPCS F4EnterCriticalSection(SoundPosSection)
#define LEAVESPCS F4LeaveCriticalSection(SoundPosSection)

int F4SoundPos::otwViewChange = 0;

F4SoundPos::F4SoundPos()
{
    uid = F4SoundPosUID;
    vel.x = 0;
    vel.y = 0;
    vel.z = 0;
    pos.x = pos.y = pos.z = 0;
    platform = 0;
    F4SoundPosUID += 100;
    inPurgeList = 0;
    inMachShadow = 0;
    sonicBoom = 0;
}

void F4SoundPos::UpdatePos(float x, float y, float z, float vx, float vy, float vz)
{
    pos.x = x;
    pos.y = y;
    pos.z = z;
    vel.x = vx;
    vel.y = vy;
    vel.z = vz;
}

void F4SoundPos::UpdatePos(SimBaseClass  *owner)
{
    platform = owner; // this should be moved to constructor

    pos.x = platform->XPos();
    pos.y = platform->YPos();
    pos.z = platform->ZPos();

    vel.x = platform->XDelta();
    vel.y = platform->YDelta();
    vel.z = platform->ZDelta();
}

void F4SoundPos::PositionalData(void)
{
    if (platform and platform->drawPointer)
    {
        orientation = ((DrawableBSP *)(platform->drawPointer))->orientation;
    }

    velocity = (float)sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);

    if (velocity)
    {
        velVec.x = vel.x / velocity;
        velVec.y = vel.y / velocity;
        velVec.z = vel.z / velocity;
    }

    int wasInMachShadow = inMachShadow;
    inMachShadow = 0;

    relPos.x = pos.x - CamPos.x;
    relPos.y = pos.y - CamPos.y;
    relPos.z = pos.z - CamPos.z;

    // dist from camera
    distance = (float)sqrt(relPos.x * relPos.x + relPos.y * relPos.y + relPos.z * relPos.z);

    if (g_bSoundSonicBoom and platform and ((SimBaseClass *)platform)->IsAirplane())
    {
        // platform is really a SimBaseClass object
        // object exceeding mach 1?
        if (velocity > 1100 and distance)
        {
            // determine the angle between the velocity vector, the object, and the camera.
            float rvx, rvy, rvz;

            rvx = relPos.x / distance;
            rvy = relPos.y / distance;
            rvz = relPos.z / distance;

            float camang = 180 * DTR - acos(velVec.x * rvx + velVec.y * rvy + velVec.z * rvz);

            float machcone = (float)atan(velocity / 1100) + 90 * DTR;
            float machcone2 = machcone + 2 * DTR;

            //MonoPrint("cones %.4f %.4f %.4f",camang * 180 / PI, machcone * 180 / PI, machcone2 * 180 / PI);

            if (camang < machcone)
            {
                inMachShadow = 1;
            }
            else
            {
                if (camang <= machcone2)
                {
                    sonicBoom = 1000; // 1000 ms
                }
            }
        }

        if (wasInMachShadow and not inMachShadow) // don't do booms with a view change
        {
            sonicBoom = 1000;
        }

        if (otwViewChange)
        {
            sonicBoom = 0;
        }

        if (sonicBoom > 0)
        {
            float v = SonicBoomTable.Lookup(1.0f - (float)sonicBoom / 1000.0f);
            sonicBoom -= (int)(SimLibMajorFrameTime * 1000);
            Sfx(SFX_SONIC_BOOM, 0, 1, v);
            //play the BOOM
        }
        else
        {
            sonicBoom = 0;
        }
    }
}


void F4SoundPos::Sfx(int SfxID, int SID, float PScale, float Vol)
{
    //F4SoundFXSetPos(SfxID, 0, pos.x, pos.y, pos.z, PScale, Vol, vel.x, vel.y, vel.z,uid + SID,platform);
    //Cobra Inhibit stuff?
    SFX_DEF_ENTRY *sfxp;

    if (SfxID <= 0 or SfxID >= NumSFX) return;

    sfxp = &SFX_DEF[ SfxID ];

    // Cobra - Fix CTD when exiting FF
    // sfr: @todo remove this hack
    if (F4IsBadReadPtr(sfxp, sizeof(sfxp))) return;

    if (g_bRealisticAvionics and (sfxp->flags bitand SFX_FLAGS_VMS))
    {
        AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

        //MI no VMS when on ground
        if (playerAC)
        {
            if (
                OTWDriver.DisplayInCockpit() and playerAC->OnGround() or
 not playerAC->playBetty or not playerAC->IsSetFlag(MOTION_OWNSHIP)
            )
            {
                // MD -- 20031125: except if the MAL/IND test button is being pressed to test the warning sound
                if ( not ((SfxID == SFX_BB_ALLWORDS) and playerAC->TestLights))
                    return;
            }
        }
    }

    //End
    Sfx(SfxID, SID, PScale, Vol, pos.x, pos.y, pos.z);
}

/*
void F4SoundPos::Sfx(int SfxID, int SID, float PScale, float Vol, float X, float Y, float Z)
{
 F4SoundFXSetPos(SfxID, 0, X, Y, Z, PScale, Vol, vel.x, vel.y, vel.z,uid + SID,platform);
}
*/

// play sounds at a location relative to object space
void F4SoundPos::SfxRel(int SfxID, int SID, float PScale, float Vol, float X, float Y, float Z)
{
    if (platform->drawPointer)
    {
        Trotation *orientation = &((DrawableBSP *)(platform->drawPointer))->orientation;

        float x = orientation->M11 * X + orientation->M12 * Y + orientation->M13 * Z + platform->XPos();
        float y = orientation->M21 * X + orientation->M22 * Y + orientation->M23 * Z + platform->YPos();
        float z = orientation->M31 * X + orientation->M32 * Y + orientation->M33 * Z + platform->ZPos();

        Sfx(SfxID, SID, PScale, Vol, x, y, z);
    }
    else
    {
        Sfx(SfxID, SID, PScale, Vol);
    }
}

void F4SoundPos::SfxRel(int SfxID, int SID, float PScale, float Vol, Tpoint &lPos)
{
    Trotation *orientation = &((DrawableBSP *)(platform->drawPointer))->orientation;

    float x = orientation->M11 * lPos.x + orientation->M12 * lPos.y + orientation->M13 * lPos.z + platform->XPos();
    float y = orientation->M21 * lPos.x + orientation->M22 * lPos.y + orientation->M23 * lPos.z + platform->YPos();
    float z = orientation->M31 * lPos.x + orientation->M32 * lPos.y + orientation->M33 * lPos.z + platform->ZPos();

    Sfx(SfxID, SID, PScale, Vol, x, y, z);
}


bool F4SoundPos::IsPlaying(int SfxID, int SID)
{
    mlrVoiceHandle *sn;

    // try to find existing node
    sn = (mlrVoiceHandle *)soList.GetHead();

    while (sn)
    {
        if (sn->AreYou(SfxID, SID))
        {
            return((bool)sn->IsPlaying());
        }

        sn = (mlrVoiceHandle *)sn->GetSucc();
    }

    return(0);

    //return (F4SoundFXPlaying(SfxID, uid + SID)>0);
}


void F4SoundPos::Sfx(int SfxID, int SID, float PScale, float Vol, float X, float Y, float Z)
{
#ifdef Prof_ENABLED // MLR 5/21/2004 - 
    Prof(F4SoundPos_Sfx);
#endif

    mlrVoiceHandle *vh;

    if ( not inPurgeList)
    {
        ENTERSPCS;
        sndPurgeList.AddHead(this);
        inPurgeList = 1;
        LEAVESPCS;
    }

    // try to find existing node
    vh = (mlrVoiceHandle *)soList.GetHead();

    while (vh)
    {
        if (vh->AreYou(SfxID, SID))
        {
            vh->Play(PScale, Vol, X, Y, Z, vel.x, vel.y, vel.z);
            return;
        }

        vh = (mlrVoiceHandle *)vh->GetSucc();
    }

    // make new node
    ShiAssert(SfxID < NumSFX);
    ShiAssert(SfxID > 0);

    if (SfxID < NumSFX and SfxID > 0)
    {
        if (vh = new mlrVoiceHandle(this, SfxID, SID))
        {
            soList.AddHead((ANode *)vh);
            vh->Play(PScale, Vol, X, Y, Z, vel.x, vel.y, vel.z);
        }
    }
}

F4SoundPos::~F4SoundPos()
{

    if (inPurgeList)
    {
        ENTERSPCS;
        inPurgeList = 0;
        Remove();
        LEAVESPCS;
    }

    mlrVoiceHandle *vh;

    while (vh = (mlrVoiceHandle *)soList.RemHead())
    {
        delete vh;
    }

}

void F4SoundPos::Purge(void)
{
    mlrVoiceHandle *sn, *sn2;

    sn = (mlrVoiceHandle *)soList.GetHead();

    while (sn)
    {
        sn2 = (mlrVoiceHandle *)sn->GetSucc();

        // delete if older than 10 seconds (arbitrary)
        if ((vuxRealTime - sn->lastPlayTime) > (10 * 1000))
        {
            sn->Remove();
            delete sn;
        }

        sn = sn2;
    }

    if ( not (soList.GetHead()))
    {
        ENTERSPCS;
        inPurgeList = 0;
        Remove();
        LEAVESPCS;
    }
}
