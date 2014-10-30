/* ------------------------------------------------------------------------

  FalcVoice.cpp

  Creates Voice Buffers with Channel

  Version 0.01

  Written by Jim DiZoglio (x257)       (c) 1997 Spectrum Holobyte
  And David Power

------------------------------------------------------------------------ */

#include <windows.h>
#include <stdio.h>
#include "f4thread.h"
#include "fsound.h"
#include "FalcVoice.h"
#include "LHSP.h"
#include "VoiceManager.h"
#include "conv.h"
#include "F4Find.h"
#include "psound.h"
#include "playerop.h"
#include "soundgroups.h"

extern VoiceManager *VM;
extern HANDLE VMWakeEventHandle;
extern WAVEFORMATEX mono_16bit_8k;

extern FILE *debugFile;
extern FILE *debugEndFile;

extern BOOL killThread;
DWORD fillVoiceBuffer(void *me, char *soundBuffer, DWORD length);

FalcVoice::FalcVoice(void)
{
    exitChannel = FALSE;
}

FalcVoice::~FalcVoice(void)
{
    CleanupVoice();
}

void FalcVoice::CreateVoice(void)
{
    voiceStruct = new VOICE;

    if (voiceStruct == NULL)
    {
        return;
    }

    silenceWritten = 0;

    /* Setup Audio Playback Buffers */
    InitializeVoiceBuffers();

    /* Initialize Flag Sets */
    voiceStruct->streamBuffer = 0;
    voiceStruct->status = QUEUE_CONV;
    // voiceStruct->currConv = 0;
    // voiceStruct->convQCount = 0;
}

void FalcVoice::InitCompressionData(void)
{
    AllocateCompInfo();

    InitCompressionFile();
}

void FalcVoice::PlayVoices(void)
{
    FalcVoiceHandle = F4CreateStream(&mono_16bit_8k, 1.0f);

    F4SetStreamVolume(FalcVoiceHandle, 0);

    F4StartCallbackStream(FalcVoiceHandle, (void *)this, fillVoiceBuffer);

    exitChannel = FALSE;
}

void SetVoiceVolumes(void)
{
    if (VM)
    {
        for (int i = 0; i < NUM_VOICE_CHANNELS; i++)
        {
            F4SetStreamVolume(VM->falconVoices[i].FalcVoiceHandle,
                              PlayerOptions.GroupVol[COM1_SOUND_GROUP + i]);
        }
    }
}

void SetVoiceVolume(int channel)
{
    if (VM and channel >= 0 and channel < NUM_VOICE_CHANNELS and not VM->falconVoices[channel].exitChannel)
    {
        F4SetStreamVolume(VM->falconVoices[channel].FalcVoiceHandle,
                          PlayerOptions.GroupVol[COM1_SOUND_GROUP + channel]);
    }
}

void FalcVoice::SilenceVoices(void)
{
    F4SetStreamVolume(FalcVoiceHandle, -10000);
    exitChannel = TRUE;
}

void FalcVoice::UnsilenceVoices(int SoundGroup)
{
    F4SetStreamVolume(FalcVoiceHandle, PlayerOptions.GroupVol[SoundGroup]);
    exitChannel = FALSE;
}

void FalcVoice::FVResumeVoiceStreams(void)
{
    if ( not F4IsSoundPlaying(FalcVoiceHandle))
    {
        F4StartCallbackStream(FalcVoiceHandle, (void *)this, fillVoiceBuffer);
        exitChannel = FALSE;
    }
}

void FalcVoice::InitializeVoiceBuffers(void)
{
    int i;

    for (i = 0; i < MAX_VOICE_BUFFERS; i++)
    {
        /* Allocate the audion buffers */
        voiceBuffers[i].waveBuffer = new unsigned char[MAX_OUTDECODE_SIZE];

        if (voiceBuffers[i].waveBuffer == NULL)
            return;

        /* Initialize Wave Format Header Information */
        InitializeVoiceStruct(i);


        voiceBuffers[i].mesgNum = 0;


        voiceBuffers[i].criticalSection = F4CreateCriticalSection("VoiceBuffer");
        voiceBuffers[i].status = BUFFER_NOT_IN_QUEUE;
    }

    /* initalize audioBuffer flags */
}

void FalcVoice::SetMesgNum(int bufferNum, int mesgNum)
{
    voiceBuffers[bufferNum].mesgNum = mesgNum;
}

void FalcVoice::InitializeVoiceStruct(int bufferNum)
{
    voiceBuffers[bufferNum].dataInWaveBuffer = 0;
    voiceBuffers[bufferNum].waveBufferLen = 0;
    voiceBuffers[bufferNum].waveBufferWrite = 0;
    voiceBuffers[bufferNum].waveBufferRead = 0;
}

void FalcVoice::InitWaveFormatEXData(WAVEFORMATEX *waveFormat)
{
    waveFormat->wFormatTag = 0x0001;
    waveFormat->nChannels = 0x0001;
    waveFormat->nSamplesPerSec = 0x00005622;
    waveFormat->nAvgBytesPerSec = 0x00005622;
    waveFormat->nBlockAlign = 0x0001;
    waveFormat->wBitsPerSample = 0x0008;
    waveFormat->cbSize = 0x0000;
}

void FalcVoice::CleanupVoice(void)
{
    int i;

    exitChannel = TRUE;

    CleanupCompressionBuffer();

    for (i = 0; i < MAX_VOICE_BUFFERS; i++)
    {
        F4DestroyCriticalSection(voiceBuffers[i].criticalSection);
        voiceBuffers[i].criticalSection = NULL;
        delete [] voiceBuffers[i].waveBuffer;
    }

    delete voiceStruct;

    voiceStruct = NULL;
}

VOICE_STREAM_BUFFER *FalcVoice::GetVoiceBuffer(int bufferNum)
{
    return(&(voiceBuffers[bufferNum]));
}

void FalcVoice::SetVoiceChannel(int channelNo)
{
    channel = channelNo;
}

//this will make all buffers for the current voice channel
//that are not in use available
void FalcVoice::PopVCAddQueue()
{
    //add the buffer that streambuffer currently points to first
    F4EnterCriticalSection(VM->vmCriticalSection);

    if (voiceBuffers[voiceStruct->streamBuffer].status == BUFFER_NOT_IN_QUEUE)
    {
        InitializeVoiceStruct(voiceStruct->streamBuffer);
        VM->VMAddBuffToQueue(channel, voiceStruct->streamBuffer);  //note: each falcvoice knows what channel it is
        voiceBuffers[voiceStruct->streamBuffer].status = BUFFER_IN_QUEUE;
    }

    if (voiceBuffers[1 - voiceStruct->streamBuffer].status == BUFFER_NOT_IN_QUEUE)
    {
        InitializeVoiceStruct(1 - voiceStruct->streamBuffer);
        VM->VMAddBuffToQueue(channel, 1 - voiceStruct->streamBuffer);  //note: each falcvoice knows what channel it is
        voiceBuffers[1 - voiceStruct->streamBuffer].status = BUFFER_IN_QUEUE;
    }

    F4LeaveCriticalSection(VM->vmCriticalSection);
}

void FalcVoice::DebugStatus(void)
{
    int i;

    for (i = 0; i < MAX_VOICE_BUFFERS; i++)
    {
        // fprintf( dbgSndFile, "Status:: buffer%d: %d\n", i, voiceBuffers[i].status );
    }
}

// Need malloc checks
void FalcVoice::AllocateCompInfo(void)
{
    voiceCompInfo = new COMPRESSION_DATA;
}

void FalcVoice::InitCompressionFile(void)
{
    voiceCompInfo->bytesDecoded = 0L;
    voiceCompInfo->bytesRead = 0L;
    voiceCompInfo->fileLength = 0L;
    voiceCompInfo->compFileLength = 0L;
}

void FalcVoice::CleanupCompressionBuffer(void)
{
    delete  voiceCompInfo;
}

void FalcVoice::BufferManager(int buffer)
{
    voiceBuffers[buffer].status = BUFFER_FILLED;
}

void FalcVoice::BufferEmpty(int buffer)
{
    voiceBuffers[buffer].status = BUFFER_NOT_IN_QUEUE;
}

void FalcVoice::ResetBufferStatus(void)
{
    int i;

    for (i = 0; i < MAX_VOICE_BUFFERS; i++)
        voiceBuffers[i].status = BUFFER_NOT_IN_QUEUE;
}

DWORD fillVoiceBuffer(void *me, char *soundBuffer, DWORD length)
{
    int fillerSize, dataToCopy, filler;
    VOICE_STREAM_BUFFER *streams;
    unsigned char *ptr, *dsb;
    FalcVoice *thisFV;

    thisFV = (FalcVoice *)me;

    if (thisFV == NULL or killThread or thisFV->exitChannel)
    {
        memset(soundBuffer, SILENCE_KEY, length);
        return length;
    }

    if (thisFV->voiceBuffers[thisFV->voiceStruct->streamBuffer].status not_eq BUFFER_FILLED)
    {
        //don't want to change buffer pointed to unless there is data in the other buffer but not
        //this one. (This way I can make sure I grab the right buffer first)
        if (thisFV->voiceBuffers[1 - thisFV->voiceStruct->streamBuffer].status not_eq BUFFER_FILLED)
        {
            // sfr: i think this is causing the buffer to stop being consumed
            /*if(gSoundDriver and (thisFV->silenceWritten > 16000) )
            {
             gSoundDriver->PauseStream(thisFV->FalcVoiceHandle);
            }*/
            memset(soundBuffer, SILENCE_KEY, length);
            thisFV->silenceWritten += length;
            return length;
            //return 0;
        }
        else
        {
            thisFV->voiceStruct->streamBuffer = 1 - thisFV->voiceStruct->streamBuffer;
        }
    }

    streams = &(thisFV->voiceBuffers[thisFV->voiceStruct->streamBuffer]);

    ptr = streams->waveBuffer + streams->waveBufferRead;
    dsb = (unsigned char *) soundBuffer;

    F4EnterCriticalSection(streams->criticalSection);

    if (streams->dataInWaveBuffer > (DWORD) length)
    {
        fillerSize = 0;
        dataToCopy = length;
    }
    else
    {
        fillerSize = length - streams->dataInWaveBuffer;
        dataToCopy = streams->dataInWaveBuffer;
    }


    if (dataToCopy)
    {
        streams->dataInWaveBuffer -= dataToCopy;
        memcpy(dsb, ptr, dataToCopy);
        ptr += dataToCopy;
        dsb += dataToCopy;
        streams->waveBufferRead += dataToCopy;
    }

    if ( not streams->dataInWaveBuffer)
    {
        thisFV->BufferEmpty(thisFV->voiceStruct->streamBuffer);
        thisFV->PopVCAddQueue();
        thisFV->voiceStruct->streamBuffer = 1 - thisFV->voiceStruct->streamBuffer;
        SetEvent(VMWakeEventHandle);
    }

    F4LeaveCriticalSection(streams->criticalSection);

    if (fillerSize)
    {
        if (thisFV->voiceBuffers[thisFV->voiceStruct->streamBuffer].status == BUFFER_FILLED)
        {
            streams = &(thisFV->voiceBuffers[thisFV->voiceStruct->streamBuffer]);
            ptr = streams->waveBuffer + streams->waveBufferRead;

            F4EnterCriticalSection(streams->criticalSection);

            if (streams->dataInWaveBuffer > (DWORD) fillerSize)
            {
                dataToCopy = fillerSize;
                fillerSize = 0;
            }
            else
            {
                fillerSize = fillerSize - streams->dataInWaveBuffer;
                dataToCopy = streams->dataInWaveBuffer;
            }

            if (dataToCopy)
            {
                streams->dataInWaveBuffer -= dataToCopy;
                memcpy(dsb, ptr, dataToCopy);
                ptr += dataToCopy;
                dsb += dataToCopy;
                streams->waveBufferRead += dataToCopy;
            }

            if ( not streams->dataInWaveBuffer)
            {
                thisFV->BufferEmpty(thisFV->voiceStruct->streamBuffer);
                thisFV->PopVCAddQueue();
                thisFV->voiceStruct->streamBuffer = 1 - thisFV->voiceStruct->streamBuffer;
                SetEvent(VMWakeEventHandle);
            }

            F4LeaveCriticalSection(streams->criticalSection);
        }
    }


    if (fillerSize)
    {
        filler = SILENCE_KEY;

        if (thisFV->voiceStruct->waveFormat.wBitsPerSample not_eq 8)
            filler = 0;

        memset(dsb, filler, fillerSize);
        thisFV->silenceWritten += fillerSize;
    }

    return length;
}
